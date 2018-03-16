#ifndef MTK_QMU_H
#define MTK_QMU_H

struct tgpd {
	u8      flag;
	u8      chksum;
	u16     databuf_len; /*Rx Allow Length*/
	struct tgpd *pnext;
	u8      *pBuf;
	u16     bufLen;
	u8      ExtLength;
	u8      ZTepFlag;
};

struct tbd {
	u8      flag;
	u8      chksum;
	u16     databuf_len; /* Rx Allow Length */
	struct  tbd *pnext;
	u8      *pBuf;
	u16     bufLen;
	u8      extLen;
	u8      reserved;
};

struct gpd_range {
	struct tbd  *pnext;
	struct tbd  *pstart;
	struct tbd  *pend;
};

#define AT_GPD_EXT_LEN          256
#define AT_BD_EXT_LEN           256
//#define MAX_GPD_NUM           30 /* we've got problem when gpd is larger */
#define MAX_GPD_NUM         4
//#define MAX_BD_NUM            500
#define MAX_BD_NUM          3
#define STRESS_IOC_TH           8
#define STRESS_GPD_TH           24
#define RANDOM_STOP_DELAY       80
#define STRESS_DATA_LENGTH      1024*64 //1024*16

#define NO_ZLP              0
#define HW_MODE             1
#define GPD_MODE            2

#define TXZLP               GPD_MODE

//#define MAX_QMU_EP            4
#define MAX_QMU_EP          1

#define TGPD_FLAGS_HWO          0x01
#define TGPD_IS_FLAGS_HWO(_pd)      (((struct tgpd *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_FLAGS_HWO(_pd)     (((struct tgpd *)_pd)->flag |= TGPD_FLAGS_HWO)
#define TGPD_CLR_FLAGS_HWO(_pd)     (((struct tgpd *)_pd)->flag &= (~TGPD_FLAGS_HWO))
#define TGPD_FORMAT_BDP         0x02
#define TGPD_IS_FORMAT_BDP(_pd)     (((struct tgpd *)_pd)->flag & TGPD_FORMAT_BDP)
#define TGPD_SET_FORMAT_BDP(_pd)    (((struct tgpd *)_pd)->flag |= TGPD_FORMAT_BDP)
#define TGPD_CLR_FORMAT_BDP(_pd)    (((struct tgpd *)_pd)->flag &= (~TGPD_FORMAT_BDP))
#define TGPD_FORMAT_BPS         0x04
#define TGPD_IS_FORMAT_BPS(_pd)     (((struct tgpd *)_pd)->flag & TGPD_FORMAT_BPS)
#define TGPD_SET_FORMAT_BPS(_pd)    (((struct tgpd *)_pd)->flag |= TGPD_FORMAT_BPS)
#define TGPD_CLR_FORMAT_BPS(_pd)    (((struct tgpd *)_pd)->flag &= (~TGPD_FORMAT_BPS))
#define TGPD_SET_FLAG(_pd, _flag)   ((struct tgpd *)_pd)->flag = (((struct tgpd *)_pd)->flag&(~TGPD_FLAGS_HWO))|(_flag)
#define TGPD_GET_FLAG(_pd)      (((struct tgpd *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_CHKSUM(_pd, _n)    ((struct tgpd *)_pd)->chksum = mu3d_hal_cal_checksum((u8 *)_pd, _n)-1
#define TGPD_SET_CHKSUM_HWO(_pd, _n)    ((struct tgpd *)_pd)->chksum = mu3d_hal_cal_checksum((u8 *)_pd, _n)-1
#define TGPD_GET_CHKSUM(_pd)        ((struct tgpd *)_pd)->chksum
#define TGPD_SET_FORMAT(_pd, _fmt)  ((struct tgpd *)_pd)->flag = (((struct tgpd *)_pd)->flag&(~TGPD_FORMAT_BDP))|(_fmt)
#define TGPD_GET_FORMAT(_pd)        ((((struct tgpd *)_pd)->flag & TGPD_FORMAT_BDP)>>1)
#define TGPD_SET_DATABUF_LEN(_pd, _len) ((struct tgpd *)_pd)->databuf_len = _len
#define TGPD_ADD_DATABUF_LEN(_pd, _len) ((struct tgpd *)_pd)->databuf_len += _len
#define TGPD_GET_DATABUF_LEN(_pd)   ((struct tgpd *)_pd)->databuf_len
#define TGPD_SET_NEXT(_pd, _next)   ((struct tgpd *)_pd)->pnext = (struct tgpd *)_next;
#define TGPD_GET_NEXT(_pd)      (struct tgpd *)((struct tgpd *)_pd)->pnext
#define TGPD_SET_TBD(_pd, _tbd)     ((struct tgpd *)_pd)->pBuf = (u8 *)_tbd;\
                    TGPD_SET_FORMAT_BDP(_pd)
#define TGPD_GET_TBD(_pd)       (struct tbd *)((struct tgpd *)_pd)->pBuf
#define TGPD_SET_DATA(_pd, _data)   ((struct tgpd *)_pd)->pBuf = (u8 *)_data
#define TGPD_GET_DATA(_pd)      (u8*)((struct tgpd *)_pd)->pBuf
#define TGPD_SET_BUF_LEN(_pd, _len)     ((struct tgpd *)_pd)->bufLen = _len
#define TGPD_ADD_BUF_LEN(_pd, _len)     ((struct tgpd *)_pd)->bufLen += _len
#define TGPD_GET_BUF_LEN(_pd)       ((struct tgpd *)_pd)->bufLen
#define TGPD_SET_EXT_LEN(_pd, _len)     ((struct tgpd *)_pd)->ExtLength = _len
#define TGPD_GET_EXT_LEN(_pd)       ((struct tgpd *)_pd)->ExtLength
#define TGPD_SET_EPaddr(_pd, _EP)   ((struct tgpd *)_pd)->ZTepFlag =(((struct tgpd *)_pd)->ZTepFlag&0xF0)|(_EP)
#define TGPD_GET_EPaddr(_pd)        ((struct tgpd *)_pd)->ZTepFlag & 0x0F
#define TGPD_FORMAT_TGL         0x10
#define TGPD_IS_FORMAT_TGL(_pd)     (((struct tgpd *)_pd)->ZTepFlag & TGPD_FORMAT_TGL)
#define TGPD_SET_FORMAT_TGL(_pd)    (((struct tgpd *)_pd)->ZTepFlag |=TGPD_FORMAT_TGL)
#define TGPD_CLR_FORMAT_TGL(_pd)    (((struct tgpd *)_pd)->ZTepFlag &= (~TGPD_FORMAT_TGL))
#define TGPD_FORMAT_ZLP         0x20
#define TGPD_IS_FORMAT_ZLP(_pd)     (((struct tgpd *)_pd)->ZTepFlag & TGPD_FORMAT_ZLP)
#define TGPD_SET_FORMAT_ZLP(_pd)    (((struct tgpd *)_pd)->ZTepFlag |=TGPD_FORMAT_ZLP)
#define TGPD_CLR_FORMAT_ZLP(_pd)    (((struct tgpd *)_pd)->ZTepFlag &= (~TGPD_FORMAT_ZLP))
#define TGPD_FORMAT_IOC         0x80
#define TGPD_IS_FORMAT_IOC(_pd)     (((struct tgpd *)_pd)->flag & TGPD_FORMAT_IOC)
#define TGPD_SET_FORMAT_IOC(_pd)    (((struct tgpd *)_pd)->flag |=TGPD_FORMAT_IOC)
#define TGPD_CLR_FORMAT_IOC(_pd)    (((struct tgpd *)_pd)->flag &= (~TGPD_FORMAT_IOC))
#define TGPD_SET_TGL(_pd, _TGL)     ((struct tgpd *)_pd)->ZTepFlag |=(( _TGL) ? 0x10: 0x00)
#define TGPD_GET_TGL(_pd)       ((struct tgpd *)_pd)->ZTepFlag & 0x10 ? 1:0
#define TGPD_SET_ZLP(_pd, _ZLP)     ((struct tgpd *)_pd)->ZTepFlag |= ((_ZLP) ? 0x20: 0x00)
#define TGPD_GET_ZLP(_pd)       ((struct tgpd *)_pd)->ZTepFlag & 0x20 ? 1:0
#define TGPD_GET_EXT(_pd)       ((u8 *)_pd + sizeof(struct tgpd))


#define TBD_FLAGS_EOL           0x01
#define TBD_IS_FLAGS_EOL(_bd)       (((struct tbd *)_bd)->flag & TBD_FLAGS_EOL)
#define TBD_SET_FLAGS_EOL(_bd)      (((struct tbd *)_bd)->flag |= TBD_FLAGS_EOL)
#define TBD_CLR_FLAGS_EOL(_bd)      (((struct tbd *)_bd)->flag &= (~TBD_FLAGS_EOL))
#define TBD_SET_FLAG(_bd, _flag)    ((struct tbd *)_bd)->flag = (u8)_flag
#define TBD_GET_FLAG(_bd)       ((struct tbd *)_bd)->flag
#define TBD_SET_CHKSUM(_pd, _n)     ((struct tbd *)_pd)->chksum = mu3d_hal_cal_checksum((u8 *)_pd, _n)
#define TBD_GET_CHKSUM(_pd)     ((struct tbd *)_pd)->chksum
#define TBD_SET_DATABUF_LEN(_pd, _len)  ((struct tbd *)_pd)->databuf_len = _len
#define TBD_GET_DATABUF_LEN(_pd)    ((struct tbd *)_pd)->databuf_len
#define TBD_SET_NEXT(_bd, _next)    ((struct tbd *)_bd)->pnext = (struct tbd *)_next
#define TBD_GET_NEXT(_bd)       (struct tbd *)((struct tbd *)_bd)->pnext
#define TBD_SET_DATA(_bd, _data)    ((struct tbd *)_bd)->pBuf = (u8 *)_data
#define TBD_GET_DATA(_bd)       (u8*)((struct tbd *)_bd)->pBuf
#define TBD_SET_BUF_LEN(_bd, _len)  ((struct tbd *)_bd)->bufLen = _len
#define TBD_ADD_BUF_LEN(_bd, _len)  ((struct tbd *)_bd)->bufLen += _len
#define TBD_GET_BUF_LEN(_bd)        ((struct tbd *)_bd)->bufLen
#define TBD_SET_EXT_LEN(_bd, _len)  ((struct tbd *)_bd)->extLen = _len
#define TBD_ADD_EXT_LEN(_bd, _len)  ((struct tbd *)_bd)->extLen += _len
#define TBD_GET_EXT_LEN(_bd)        ((struct tbd *)_bd)->extLen
#define TBD_GET_EXT(_bd)        ((u8 *)_bd + sizeof(struct tbd))

#ifdef MTK_QMU_DRV_EXT
#define EXTERN
#else
#define EXTERN extern
#endif

#if 0
EXTERN u8 is_bdp;
EXTERN u32 gpd_buf_size;
EXTERN u16 bd_buf_size;
EXTERN u8 bd_extension;
EXTERN u8 gpd_extension;
EXTERN u32 g_dma_buffer_size;
EXTERN struct tgpd *rx_gpd_head[15];
EXTERN struct tgpd *tx_gpd_head[15];
EXTERN struct tgpd *rx_gpd_end[15];
EXTERN struct tgpd *tx_gpd_end[15];
EXTERN struct tgpd *rx_gpd_last[15];
EXTERN struct tgpd *tx_gpd_last[15];
EXTERN struct gpd_range rx_gpd_list[15];
EXTERN struct gpd_range tx_gpd_list[15];
EXTERN struct gpd_range rx_bd_list[15];
EXTERN struct gpd_range tx_bd_list[15];
EXTERN u32 rx_gpd_offset[15];
EXTERN u32 tx_gpd_offset[15];

EXTERN u32 rx_bd_offset[15];
EXTERN u32 tx_bd_offset[15];
#else
u8 is_bdp;
u32 gpd_buf_size;
u16 bd_buf_size;
u8 bd_extension;
u8 gpd_extension;
u32 g_dma_buffer_size;
struct tgpd *rx_gpd_head[15];
struct tgpd *tx_gpd_head[15];
struct tgpd *rx_gpd_end[15];
struct tgpd *tx_gpd_end[15];
struct tgpd *rx_gpd_last[15];
struct tgpd *tx_gpd_last[15];
struct gpd_range rx_gpd_list[15] __attribute__((aligned(64)));
struct gpd_range tx_gpd_list[15] __attribute__((aligned(64)));
struct gpd_range rx_bd_list[15] __attribute__((aligned(64)));
struct gpd_range tx_bd_list[15] __attribute__((aligned(64)));

u32 rx_gpd_offset[15];
u32 tx_gpd_offset[15];

u32 rx_bd_offset[15];
u32 tx_bd_offset[15];
#endif

EXTERN void mu3d_hal_resume_qmu(int q_ep_num,  u8 dir);
EXTERN void mu3d_hal_stop_qmu(int q_ep_num,  u8 dir);
EXTERN struct tgpd* mu3d_hal_prepare_tx_gpd(struct tgpd* gpd, u8 *pBuf, u32 data_length, u8 ep_num, u8 _is_bdp, u8 isHWO,u8 ioc, u8 bps,u8 zlp);
EXTERN struct tgpd* mu3d_hal_prepare_rx_gpd(struct tgpd* gpd, u8 *pBuf, u32 data_len, u8 ep_num, u8 _is_bdp, u8 isHWO, u8 ioc, u8 bps,u32 c_maxpktsz);
EXTERN void mu3d_hal_insert_transfer_gpd(int ep_num,u8 dir, u8 *buf, u32 count, u8 isHWO, u8 ioc, u8 bps,u8 zlp, u32 c_maxpktsz);
EXTERN void mu3d_hal_alloc_qmu_mem(void);
EXTERN void mu3d_hal_init_qmu(void);
EXTERN void mu3d_hal_start_qmu(int q_ep_num, u8 dir);
EXTERN void mu3d_hal_flush_qmu(int q_ep_num, u8 dir);
EXTERN void mu3d_hal_restart_qmu(int q_ep_num, u8 dir);
EXTERN void mu3d_hal_reset_qmu_ep(int q_ep_num, u8 dir);
EXTERN void mu3d_hal_send_stall(int q_ep_num, u8 dir);
EXTERN u8 mu3d_hal_cal_checksum(u8 *data, int len);
EXTERN void *mu3d_hal_gpd_virt_to_phys(void *vaddr, u8 dir, u32 num);

#endif
