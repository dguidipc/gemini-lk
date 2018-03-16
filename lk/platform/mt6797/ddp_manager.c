#define LOG_TAG "ddp_manager"

#include <platform/ddp_reg.h>

#include <platform/ddp_path.h>
#include <platform/ddp_manager.h>
#include <platform/ddp_log.h>


#define DDP_MAX_MANAGER_HANDLE (DISP_MUTEX_DDP_COUNT+DISP_MUTEX_DDP_FIRST)
#define DEFAULT_IRQ_EVENT_SCENARIO (3)

typedef struct {
	unsigned int       init;
	DISP_PATH_EVENT    event;
	//wait_queue_head_t  wq;
	unsigned long long data;
} DPMGR_WQ_HANDLE;

typedef struct {
	DDP_IRQ_BIT         irq_bit;
} DDP_IRQ_EVENT_MAPPING;

typedef struct {
	cmdqRecHandle           cmdqhandle;
	int                     hwmutexid;
	int                     power_sate;
	DDP_MODE                mode;
	//struct mutex            mutex_lock;
	DDP_IRQ_EVENT_MAPPING   irq_event_map[DISP_PATH_EVENT_NUM];
	DPMGR_WQ_HANDLE         wq_list[DISP_PATH_EVENT_NUM];
	DDP_SCENARIO_ENUM       scenario;
	disp_ddp_path_config    last_config;
} ddp_path_handle_t, *ddp_path_handle;

typedef struct {
	int                     handle_cnt;
	int                     mutex_idx;
	int                     power_sate;
	//struct mutex            mutex_lock;
	DISP_MODULE_ENUM        module_usage_table[DISP_MODULE_NUM];
	ddp_path_handle         module_path_table[DISP_MODULE_NUM];
	ddp_path_handle         handle_pool[DDP_MAX_MANAGER_HANDLE];
} DDP_MANAGER_CONTEXT;

extern DDP_MODULE_DRIVER  * ddp_modules_driver[DISP_MODULE_NUM];
static ddp_path_handle_t g_handle;

static DDP_IRQ_EVENT_MAPPING ddp_irq_event_list[DEFAULT_IRQ_EVENT_SCENARIO][DISP_PATH_EVENT_NUM] = {
	{
		//ovl0 path
		{DDP_IRQ_RDMA0_START         }, /*FRAME_START*/
		{DDP_IRQ_RDMA0_DONE          }, /*FRAME_DONE*/
		{DDP_IRQ_RDMA0_REG_UPDATE    }, /*FRAME_REG_UPDATE*/
		{DDP_IRQ_RDMA0_TARGET_LINE   }, /*FRAME_TARGET_LINE*/
		{DDP_IRQ_WDMA0_FRAME_COMPLETE}, /*FRAME_COMPLETE*/
		{DDP_IRQ_RDMA0_TARGET_LINE   }, /*FRAME_STOP*/
		{DDP_IRQ_RDMA0_REG_UPDATE    }, /*IF_CMD_DONE*/
		{DDP_IRQ_DSI0_EXT_TE         }, /*IF_VSYNC*/
		{DDP_IRQ_UNKNOW              }, /*AAL_TRIGER*/
		{DDP_IRQ_UNKNOW              }, /*COLOR_TRIGER*/
	},
	{
		//ovl1 path
		{DDP_IRQ_RDMA1_START         }, /*FRAME_START*/
		{DDP_IRQ_RDMA1_DONE          }, /*FRAME_DONE*/
		{DDP_IRQ_RDMA1_REG_UPDATE    }, /*FRAME_REG_UPDATE*/
		{DDP_IRQ_RDMA1_TARGET_LINE   }, /*FRAME_TARGET_LINE*/
		{DDP_IRQ_WDMA1_FRAME_COMPLETE}, /*FRAME_COMPLETE*/
		{DDP_IRQ_RDMA1_TARGET_LINE   }, /*FRAME_STOP*/
		{DDP_IRQ_RDMA1_REG_UPDATE    }, /*IF_CMD_DONE*/
		{DDP_IRQ_RDMA1_TARGET_LINE   }, /*IF_VSYNC*/
		{DDP_IRQ_UNKNOW              }, /*AAL_TRIGER*/
		{DDP_IRQ_UNKNOW              }, /*COLOR_TRIGER*/
	},
	{
		// rdma path
		{DDP_IRQ_RDMA2_START         }, /*FRAME_START*/
		{DDP_IRQ_RDMA2_DONE          }, /*FRAME_DONE*/
		{DDP_IRQ_RDMA2_REG_UPDATE    }, /*FRAME_REG_UPDATE*/
		{DDP_IRQ_RDMA2_TARGET_LINE   }, /*FRAME_TARGET_LINE*/
		{DDP_IRQ_UNKNOW              }, /*FRAME_COMPLETE*/
		{DDP_IRQ_RDMA2_TARGET_LINE   }, /*FRAME_STOP*/
		{DDP_IRQ_RDMA2_REG_UPDATE    }, /*IF_CMD_DONE*/
		{DDP_IRQ_RDMA2_TARGET_LINE   }, /*IF_VSYNC*/
		{DDP_IRQ_UNKNOW              }, /*AAL_TRIGER*/
		{DDP_IRQ_UNKNOW              }, /*COLOR_TRIGER*/
	},
};

static DDP_MANAGER_CONTEXT* _get_context(void)
{
	static int is_context_inited = 0;
	static DDP_MANAGER_CONTEXT context;
	if (!is_context_inited) {
		memset((void*)&context, 0, sizeof(DDP_MANAGER_CONTEXT));
		context.mutex_idx = (1<<DISP_MUTEX_DDP_COUNT) - 1;
		//mutex_init(&context.mutex_lock);
		is_context_inited = 1;
	}
	return &context;
}

static int path_top_clock_off()
{
	int i =0;
	DDP_MANAGER_CONTEXT * context = _get_context();
	if (context->power_sate) {
		for (i=0; i< DDP_MAX_MANAGER_HANDLE; i++) {
			if (context->handle_pool[i] != NULL && context->handle_pool[i]->power_sate !=0) {
				return 0;
			}
		}
		context->power_sate = 0;
		ddp_path_top_clock_off();
	}
}

static int path_top_clock_on()
{
	DDP_MANAGER_CONTEXT * context = _get_context();
	if (!context->power_sate) {
		context->power_sate = 1;
		ddp_path_top_clock_on();
	}
	return 0;
}

static ddp_path_handle find_handle_by_module(DISP_MODULE_ENUM module)
{
	return _get_context()->module_path_table[module];
}

static int dpmgr_module_notify(DISP_MODULE_ENUM module, DISP_PATH_EVENT event)
{
	ddp_path_handle handle = find_handle_by_module(module);
	return dpmgr_signal_event(handle,event);
}

static int assign_default_irqs_table(DDP_SCENARIO_ENUM scenario,DDP_IRQ_EVENT_MAPPING * irq_events)
{
	int idx = 0;
	switch (scenario) {
		case DDP_SCENARIO_PRIMARY_DISP:
		case DDP_SCENARIO_PRIMARY_OVL_MEMOUT:
		case DDP_SCENARIO_PRIMARY_ALL:
		case DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP:
		case DDP_SCENARIO_PRIMARY_RDMA0_DISP:
		case DDP_SCENARIO_DISPLAY_INTERFACE:
			idx = 0;
			break;
		case DDP_SCENARIO_SUB_DISP:
		case DDP_SCENARIO_SUB_OVL_MEMOUT:
		case DDP_SCENARIO_SUB_ALL:
		case DDP_SCENARIO_SUB_RDMA1_DISP:
			idx = 1;
			break;
		default:
			DISP_LOG_E("unknow scenario %d\n",scenario);
	}
	DISP_LOG_I("assign default irqs table index %d\n",idx);
	memcpy(irq_events,ddp_irq_event_list[idx],sizeof(ddp_irq_event_list[idx]));
	return 0;
}

static int acquire_free_bit(unsigned int total)
{
	int free_id =0 ;
	while (total) {
		if (total & 0x1) {
			return free_id;
		}
		total>>=1;
		++free_id;
	}
	return -1;
}

static int acquire_mutex(DDP_SCENARIO_ENUM scenario)
{
///: primay use mutex 0
	int mutex_id =0 ;
	DDP_MANAGER_CONTEXT * content = _get_context();
	int mutex_idx_free = content->mutex_idx;
	ASSERT(scenario >= 0  && scenario < DDP_SCENARIO_MAX);
	while (mutex_idx_free) {
		if (mutex_idx_free & 0x1) {
			content->mutex_idx &= (~(0x1<<mutex_id));
			mutex_id += DISP_MUTEX_DDP_FIRST;
			break;
		}
		mutex_idx_free>>=1;
		++mutex_id;
	}
	ASSERT(mutex_id < (DISP_MUTEX_DDP_FIRST+DISP_MUTEX_DDP_COUNT));
	DISP_LOG_I("scenario %s acquire mutex %d , left mutex 0x%x!\n",
	           ddp_get_scenario_name(scenario), mutex_id, content->mutex_idx);
	return mutex_id;
}

static int release_mutex(int mutex_idx)
{
	DDP_MANAGER_CONTEXT * content = _get_context();
	ASSERT(mutex_idx < (DISP_MUTEX_DDP_FIRST+DISP_MUTEX_DDP_COUNT));
	content->mutex_idx |= 1<< (mutex_idx-DISP_MUTEX_DDP_FIRST);
	DISP_LOG_I("release mutex %d , left mutex 0x%x!\n",
	           mutex_idx, content->mutex_idx);
	return 0;
}

int dpmgr_path_set_video_mode(disp_path_handle dp_handle, int is_vdo_mode)
{
	ddp_path_handle handle = NULL;
	ASSERT(dp_handle != NULL);
	handle = (ddp_path_handle)dp_handle;
	handle->mode = is_vdo_mode ?  DDP_VIDEO_MODE : DDP_CMD_MODE;
	DISP_LOG_I("set scenario %s mode %s\n",ddp_get_scenario_name(handle->scenario),
	           is_vdo_mode ? "Video Mode":"Cmd Mode");
	return 0;
}

static int _dpmgr_path_connect(DDP_SCENARIO_ENUM scenario, void *handle)
{
	int i = 0, module;
	int *modules = ddp_get_scenario_list(scenario);
	int module_num = ddp_get_module_num(scenario);

	ddp_connect_path(scenario, handle);

	for (i = 0; i < module_num; i++) {
		module = modules[i];
		if (ddp_modules_driver[module] && ddp_modules_driver[module]->connect) {
			int prev = i == 0 ? DISP_MODULE_UNKNOWN : modules[i - 1];
			int next = i == module_num - 1 ? DISP_MODULE_UNKNOWN : modules[i + 1];

			ddp_modules_driver[module]->connect(module, prev, next, 1, handle);
		}
	}

	return 0;
}

static int _dpmgr_path_disconnect(DDP_SCENARIO_ENUM scenario, void *handle)
{
	int i = 0, module;
	int *modules = ddp_get_scenario_list(scenario);
	int module_num = ddp_get_module_num(scenario);

	ddp_disconnect_path(scenario, handle);

	for (i = 0; i < module_num; i++) {
		module = modules[i];
		if (ddp_modules_driver[module] && ddp_modules_driver[module]->connect) {
			int prev = i == 0 ? DISP_MODULE_UNKNOWN : modules[i - 1];
			int next = i == module_num - 1 ? DISP_MODULE_UNKNOWN : modules[i + 1];

			ddp_modules_driver[module]->connect(module, prev, next, 1, handle);
		}
	}

	return 0;
}

disp_path_handle dpmgr_create_path(DDP_SCENARIO_ENUM scenario, cmdqRecHandle cmdq_handle)
{
	int i =0;
	int module_name ;
	ddp_path_handle path_handle = NULL;
	int *modules = ddp_get_scenario_list(scenario);
	int module_num = ddp_get_module_num(scenario);
	DDP_MANAGER_CONTEXT *content = _get_context();

	//path_handle = kzalloc(sizeof(uint8_t*) * sizeof(ddp_path_handle_t), GFP_KERNEL);
	memset((void *)(&g_handle), 0, sizeof(ddp_path_handle_t));
	path_handle  = &g_handle;
	if (NULL != path_handle) {
		path_handle->cmdqhandle = cmdq_handle;
		path_handle->scenario = scenario;
		path_handle->hwmutexid = acquire_mutex(scenario);
		assign_default_irqs_table(scenario,path_handle->irq_event_map);
		DISP_LOG_I("create handle 0x%x on scenario %s\n",path_handle,ddp_get_scenario_name(scenario));
		for (i=0; i< module_num; i++) {
			module_name = modules[i];
			content->module_usage_table[module_name]++;
			content->module_path_table[module_name] = path_handle;
		}
		content->handle_cnt++;
		content->handle_pool[path_handle->hwmutexid] = path_handle;
	} else {
		DISP_LOG_E("Fail to create handle on scenario %s\n",ddp_get_scenario_name(scenario));
	}
	return path_handle;
}

int dpmgr_destroy_path(disp_path_handle dp_handle)
{
	int i =0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	DDP_MANAGER_CONTEXT * content = _get_context();
	DISP_LOG_I("destroy path handle 0x%x on scenario %s\n",handle,ddp_get_scenario_name(handle->scenario));
	if (handle != NULL) {
		release_mutex(handle->hwmutexid);
		_dpmgr_path_disconnect(handle->scenario,handle->cmdqhandle);
		for ( i=0; i< module_num; i++) {
			module_name = modules[i];
			content->module_usage_table[module_name]--;
			content->module_path_table[module_name] = NULL;
		}
		content->handle_cnt --;
		ASSERT(content->handle_cnt >=0);
		content->handle_pool[handle->hwmutexid] = NULL;
		// kfree(handle);
	}
	return 0;
}

int dpmgr_path_add_dump(disp_path_handle dp_handle, ENGINE_DUMP engine,int encmdq )
{
	int ret = 0;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	ASSERT(handle->scenario ==DDP_SCENARIO_PRIMARY_DISP || handle->scenario ==DDP_SCENARIO_SUB_DISP);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_MODULE_ENUM wdma = ddp_is_scenario_on_primary(handle->scenario) ? DISP_MODULE_WDMA0: DISP_MODULE_WDMA1;
	// update contxt
	DDP_MANAGER_CONTEXT * context = _get_context();
	context->module_usage_table[wdma]++;
	context->module_path_table[wdma] = handle;
	handle->scenario = DDP_SCENARIO_PRIMARY_ALL;
	// update connected
	_dpmgr_path_connect(handle->scenario,cmdqHandle);
	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdqHandle);

	//wdma just need start.
	if (ddp_modules_driver[wdma] != 0) {
		if (ddp_modules_driver[wdma]->init!= 0) {
			ddp_modules_driver[wdma]->init(wdma, cmdqHandle);
		}
		if (ddp_modules_driver[wdma]->start!= 0) {
			ddp_modules_driver[wdma]->start(wdma, cmdqHandle);
		}
	}
	return 0;
}

int dpmgr_path_remove_dump(disp_path_handle dp_handle, int encmdq )
{
	int ret = 0;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	ASSERT(handle->scenario ==DDP_SCENARIO_PRIMARY_DISP || handle->scenario ==DDP_SCENARIO_PRIMARY_ALL
	       || handle->scenario ==DDP_SCENARIO_SUB_DISP || handle->scenario ==DDP_SCENARIO_SUB_ALL);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_MODULE_ENUM wdma = ddp_is_scenario_on_primary(handle->scenario) ? DISP_MODULE_WDMA0: DISP_MODULE_WDMA1;
	// update contxt
	DDP_MANAGER_CONTEXT * context = _get_context();
	context->module_usage_table[wdma]--;
	context->module_path_table[wdma] = 0;
	//wdma just need stop
	if (ddp_modules_driver[wdma] != 0) {
		if (ddp_modules_driver[wdma]->stop!= 0) {
			ddp_modules_driver[wdma]->stop(wdma, cmdqHandle);
		}
		if (ddp_modules_driver[wdma]->deinit!= 0) {
			ddp_modules_driver[wdma]->deinit(wdma, cmdqHandle);
		}
	}
	_dpmgr_path_disconnect(DDP_SCENARIO_PRIMARY_OVL_MEMOUT,cmdqHandle);

	handle->scenario = DDP_SCENARIO_PRIMARY_DISP;
	// update connected
	_dpmgr_path_connect(handle->scenario,cmdqHandle);
	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdqHandle);

	return 0;
}

int dpmgr_path_set_dst_module(disp_path_handle dp_handle,DISP_MODULE_ENUM dst_module)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	ASSERT((handle->scenario >= 0  && handle->scenario < DDP_SCENARIO_MAX));
	DISP_LOG_I("set dst module on scenario %s, module %s\n",
	           ddp_get_scenario_name(handle->scenario),ddp_get_module_name(dst_module));
	return ddp_set_dst_module(handle->scenario, dst_module);
}

DISP_MODULE_ENUM dpmgr_path_get_dst_module(disp_path_handle dp_handle)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	ASSERT((handle->scenario >= 0  && handle->scenario < DDP_SCENARIO_MAX));
	DISP_MODULE_ENUM dst_module = ddp_get_dst_module(handle->scenario);
	DISP_LOG_I("get dst module on scenario %s, module %s\n",
	           ddp_get_scenario_name(handle->scenario),ddp_get_module_name(dst_module));
	return dst_module;
}

int  dpmgr_path_init(disp_path_handle dp_handle, int encmdq)
{
	int i=0;
	int module_name;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DISP_LOG_I("path init on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	//turn off m4u
	ddp_path_m4u_off();
	//open top clock
	ddp_path_top_clock_on();
	//seting mutex
	ddp_mutex_set(handle->hwmutexid,
	              handle->scenario,
	              handle->mode,
	              cmdqHandle);
	//connect path;
	_dpmgr_path_connect(handle->scenario, cmdqHandle);

	// each module init
	for (i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->init!= 0) {
				ddp_modules_driver[module_name]->init(module_name, cmdqHandle);
			}
			if (ddp_modules_driver[module_name]->set_listener!= 0) {
				ddp_modules_driver[module_name]->set_listener(module_name,dpmgr_module_notify);
			}
		}
	}
	//after init this path will power on;
	handle->power_sate = 1;
	return 0;
}

int  dpmgr_path_deinit(disp_path_handle dp_handle, int encmdq)
{
	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_LOG_I("path deinit on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	ddp_mutex_clear(handle->hwmutexid,cmdqHandle);
	_dpmgr_path_disconnect(handle->scenario, cmdqHandle);
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->deinit!= 0) {
				DISP_LOG_V("scenario %s deinit module  %s\n",ddp_get_scenario_name(handle->scenario),
				           ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->deinit(module_name, cmdqHandle);
			}
			if (ddp_modules_driver[module_name]->set_listener!= 0) {
				ddp_modules_driver[module_name]->set_listener(module_name,NULL);
			}
		}
	}
	handle->power_sate = 0;
	//close top clock when last path init
	path_top_clock_off();
	return 0;
}

int dpmgr_path_start(disp_path_handle dp_handle, int encmdq)
{
	int i=0;
	int module_name;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DISP_LOG_V("path start on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->start!= 0) {
				ddp_modules_driver[module_name]->start(module_name, cmdqHandle);
			}
		}
	}
	return 0;
}

int dpmgr_path_stop(disp_path_handle dp_handle, int encmdq)
{

	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_LOG_I("path stop on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->stop!= 0) {
				DISP_LOG_V("scenario %s  stop module %s \n",ddp_get_scenario_name(handle->scenario),
				           ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->stop(module_name, cmdqHandle);
			}
		}
	}
	return 0;
}

int dpmgr_path_reset(disp_path_handle dp_handle, int encmdq)
{
	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_LOG_I("path reset on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->reset != 0) {
				DISP_LOG_V("scenario %s  reset module %s \n",ddp_get_scenario_name(handle->scenario),
				           ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->reset(module_name, cmdqHandle);
			}
		}
	}
	return 0;
}

int dpmgr_path_config(disp_path_handle dp_handle, disp_ddp_path_config *config, int encmdq)
{
	int i=0;
	int module_name;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DISP_LOG_V("path config ovl %d, rdma %d, wdma %d, dst %d on handle 0x%x scenario %s \n",
	           config->ovl_dirty,
	           config->rdma_dirty,
	           config->wdma_dirty,
	           config->dst_dirty,
	           handle,
	           ddp_get_scenario_name(handle->scenario));
	memcpy(&handle->last_config, config, sizeof(disp_ddp_path_config));
	for (i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->config!= 0) {
				ddp_modules_driver[module_name]->config(module_name, config, cmdqHandle);
			}
		}
	}
	return 0;
}

disp_ddp_path_config *dpmgr_path_get_last_config(disp_path_handle dp_handle)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	handle->last_config.ovl_dirty  =0;
	handle->last_config.rdma_dirty =0;
	handle->last_config.wdma_dirty =0;
	handle->last_config.dst_dirty =0;
	return &handle->last_config;
}

int dpmgr_path_build_cmdq(disp_path_handle dp_handle, void *trigger_loop_handle, CMDQ_STATE state)
{
	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_D("path build cmdq on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->build_cmdq!= 0) {
				DISP_LOG_D("%s build cmdq, state=%d\n",ddp_get_module_name(module_name), state);
				ddp_modules_driver[module_name]->build_cmdq(module_name, trigger_loop_handle, state); // now just 0;
			}
		}
	}
	return 0;
}

int dpmgr_path_trigger(disp_path_handle dp_handle, void *trigger_loop_handle, int encmdq)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DISP_LOG_V("dpmgr_path_trigger on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	int i=0;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	int module_name;

	ddp_mutex_enable(handle->hwmutexid,handle->scenario,trigger_loop_handle);

	for (i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->trigger!= 0) {
				ddp_modules_driver[module_name]->trigger(module_name, trigger_loop_handle);
			}
		}
	}

	return 0;
}

int dpmgr_path_flush(disp_path_handle dp_handle,int encmdq)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	cmdqRecHandle cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DISP_LOG_I("path flush on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	return ddp_mutex_enable(handle->hwmutexid,handle->scenario,cmdqHandle);
}

int dpmgr_path_power_off(disp_path_handle dp_handle, CMDQ_SWITCH encmdq)
{
	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	DISP_LOG_I("path power off on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->power_off!= 0) {
				DISP_LOG_I(" %s power off\n",ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->power_off(module_name, encmdq?handle->cmdqhandle:NULL); // now just 0;
			}
		}
	}
	handle->power_sate = 0;
	path_top_clock_off();
	return 0;
}

int dpmgr_path_power_on(disp_path_handle dp_handle, CMDQ_SWITCH encmdq)
{
	int i=0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	DISP_LOG_I("path power on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	path_top_clock_on();
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->power_on!= 0) {
				DISP_LOG_I("%s power on\n",ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->power_on(module_name, encmdq?handle->cmdqhandle:NULL); // now just 0;
			}
		}
	}
	//modules on this path will resume power on;
	handle->power_sate = 1;
	return 0;
}

int dpmgr_path_set_parameter(disp_path_handle dp_handle, int io_evnet, void * data)
{
	return 0;
}

int dpmgr_path_get_parameter(disp_path_handle dp_handle,int io_evnet, void * data )
{
	return 0;
}

int dpmgr_path_is_idle(disp_path_handle dp_handle)
{
	ASSERT(dp_handle != NULL);
	return !dpmgr_path_is_busy(dp_handle);
}

int dpmgr_path_is_busy(disp_path_handle dp_handle)
{
	int i=0;
	int module_name;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_V("path check busy on scenario %s\n",ddp_get_scenario_name(handle->scenario));
	for (i=module_num-1; i>=0; i--) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->is_busy!= 0) {
				if (ddp_modules_driver[module_name]->is_busy(module_name)) {
					DISP_LOG_V("%s is busy\n", ddp_get_module_name(module_name));
					return 1;
				}
			}
		}
	}
	return 0;
}

int dpmgr_set_lcm_utils(disp_path_handle dp_handle, void *lcm_drv)
{
	int i=0;
	int module_name;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_V("path set lcm drv handle 0x%x\n",handle);
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if ((ddp_modules_driver[module_name]->set_lcm_utils!= 0)&&lcm_drv) {
				DISP_LOG_I("%s set lcm utils\n",ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->set_lcm_utils(module_name, lcm_drv); // now just 0;
			}
		}
	}
	return 0;
}

int dpmgr_enable_event(disp_path_handle dp_handle, DISP_PATH_EVENT event)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DPMGR_WQ_HANDLE *wq_handle = &handle->wq_list[event];

	DISP_LOG_I("enable event on scenario %s, event %d, irtbit 0x%x\n",
	           ddp_get_scenario_name(handle->scenario),
	           event,
	           handle->irq_event_map[event].irq_bit);
	if (!wq_handle->init) {
		//init_waitqueue_head(&(wq_handle->wq));
		wq_handle->init = 1;
		wq_handle->data= 0;
		wq_handle->event = event;
	}
	return 0;
}

int dpmgr_map_event_to_irq(disp_path_handle dp_handle, DISP_PATH_EVENT event, DDP_IRQ_BIT irq_bit)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DDP_IRQ_EVENT_MAPPING *irq_table = handle->irq_event_map;

	if (event < DISP_PATH_EVENT_NUM) {
		DISP_LOG_I("map event %d to irq 0x%x on scenario %s\n",event,irq_bit,ddp_get_scenario_name(handle->scenario));
		irq_table[event].irq_bit= irq_bit;
		return 0;
	}
	DISP_LOG_E("fail to map event %d to irq 0x%x on scenario %s\n",event,irq_bit,ddp_get_scenario_name(handle->scenario));
	return -1;
}


int dpmgr_disable_event(disp_path_handle dp_handle, DISP_PATH_EVENT event)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DISP_LOG_I("disable event %d on scenario %s \n",event,ddp_get_scenario_name(handle->scenario));
	DPMGR_WQ_HANDLE *wq_handle = &handle->wq_list[event];
	wq_handle->init = 0;
	wq_handle->data= 0;
	return 0;
}

int dpmgr_check_status(disp_path_handle dp_handle)
{
	int i =0;
	int module_name ;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	int * modules = ddp_get_scenario_list(handle->scenario);
	int module_num = ddp_get_module_num(handle->scenario);
	DISP_LOG_I("check status on scenario %s, module_num %d \n",
	           ddp_get_scenario_name(handle->scenario),module_num);
	ddp_check_path(handle->scenario);
	ddp_check_mutex(handle->hwmutexid,handle->scenario,
	                handle->mode);
	//ddp_check_signal(handle->scenario);
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		ddp_dump_analysis(module_name);

		if (i == DISP_MODULE_DSI0 || i == DISP_MODULE_DSI1 || i == DISP_MODULE_DSIDUAL) {
			if (ddp_modules_driver[i] != 0) {
				if (ddp_modules_driver[i]->dump_info!= 0) {
					ddp_modules_driver[i]->dump_info(i, 1);
				}
			}
		}
	}
	ddp_dump_analysis(DISP_MODULE_CONFIG);
	ddp_dump_analysis(DISP_MODULE_MUTEX);
	for ( i=0; i< module_num; i++) {
		module_name = modules[i];
		ddp_dump_reg(module_name);
	}
	ddp_dump_reg(DISP_MODULE_CONFIG);
	ddp_dump_reg(DISP_MODULE_MUTEX);
	return 0;
}

int dpmgr_wait_event_timeout(disp_path_handle dp_handle, DISP_PATH_EVENT event, int timeout)
{
	int ret = -1;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DPMGR_WQ_HANDLE *wq_handle = &handle->wq_list[event];
	DDP_IRQ_EVENT_MAPPING *irqmap = handle->irq_event_map;
	DISP_MODULE_ENUM module = IRQBIT_MODULE(irqmap[event].irq_bit);
	int q_bit = IRQBIT_BIT(irqmap[event].irq_bit);

	if (wq_handle->init) {
		//DISP_LOG_V("wait event %d timeout %d on scenario %s\n", event,timeout,ddp_get_scenario_name(handle->scenario));
		//should poling
		if (ddp_modules_driver[module] !=0 && ddp_modules_driver[module]->polling_irq != 0) {
			ret = ddp_modules_driver[module]->polling_irq(module, q_bit, timeout);
		}
		if (ret > 0) {
			//DISP_LOG_V("received event  %d  left time %d on scenario %s \n", event, ret,ddp_get_scenario_name(handle->scenario));

		} else {
			DISP_LOG_E("wait %d timeout ret %d on scenario %d \n", event, ret, handle->scenario);
			dpmgr_check_status(handle);
		}
		return ret;
	}
	DISP_LOG_E("wait event %d not initialized on scenario %s\n", event,ddp_get_scenario_name(handle->scenario));
	return ret;
}

int dpmgr_wait_event(disp_path_handle dp_handle, DISP_PATH_EVENT event)
{
	int ret = -1;
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DPMGR_WQ_HANDLE *wq_handle = & handle->wq_list[event];
	DDP_IRQ_EVENT_MAPPING *irqmap = handle->irq_event_map;
	DISP_MODULE_ENUM module = IRQBIT_MODULE(irqmap[event].irq_bit);
	int q_bit = IRQBIT_BIT(irqmap[event].irq_bit);

	if (wq_handle->init) {
		DISP_LOG_V("wait event %d on scenario %s\n", event,ddp_get_scenario_name(handle->scenario));
		if (ddp_modules_driver[module] !=0 && ddp_modules_driver[module]->polling_irq != 0) {
			ret = ddp_modules_driver[module]->polling_irq(module,q_bit,-1);
		}
		DISP_LOG_V("received event  %d on scenario %s \n", event, ddp_get_scenario_name(handle->scenario));
		return ret;
	}
	DISP_LOG_E("wait event %d not initialized on scenario %s\n", event,ddp_get_scenario_name(handle->scenario));
	return ret;
}

int dpmgr_signal_event(disp_path_handle dp_handle, DISP_PATH_EVENT event)
{
	ASSERT(dp_handle != NULL);
	ddp_path_handle handle = (ddp_path_handle)dp_handle;
	DPMGR_WQ_HANDLE *wq_handle = &handle->wq_list[event];
	if (handle->wq_list[event].init) {
		//wq_handle->data = sched_clock();
		DISP_LOG_V("wake up evnet %d on scenario %s\n", event,ddp_get_scenario_name(handle->scenario));
		//wake_up_interruptible(&(handle->wq_list[event].wq));
	}
	return 0;
}

int dpmgr_init(void)
{
	DISP_LOG_I("ddp manager init\n");
	return 0;
}

/*
static void dpmgr_irq_handler(DISP_MODULE_ENUM module,unsigned int regvalue)
{
    int i = 0;
    int j = 0;
    int irq_bits_num =0;
    int irq_bit = 0;
    ddp_path_handle handle = NULL;
    DISP_PATH_EVENT event = DISP_PATH_EVENT_NONE;
    DISP_LOG_V("irq handler module %d, regvalue 0x%x\n", module,regvalue);
    handle   = find_handle_by_module(module);
    if(handle == NULL)
    {
        return;
    }
    irq_bits_num =  ddp_get_module_max_irq_bit(module);
    for(i = 0; i<=irq_bits_num; i++)
    {
        if(regvalue & (0x1<<i))
        {
            irq_bit = (module<<16 | 0x1<<i);
            for(j = 0; j < DISP_PATH_EVENT_NUM; j++)
            {
                if(handle->wq_list[j].init && irq_bit ==  handle->irq_event_map[j].irq_bit)
                {
                    //handle->wq_list[j].data =sched_clock();
                    DISP_LOG_V("irq signal event %d on cycle %llu on scenario %s\n",
                                event, handle->wq_list[j].data ,ddp_get_scenario_name(handle->scenario));
                    //wake_up_interruptible(&(handle->wq_list[j].wq));
                }
            }
        }
    }
    return ;
}
*/
