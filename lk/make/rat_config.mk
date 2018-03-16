#######################################################################
#
#AUTO generate suitable feature options for MTK_PROTOCOL1_RAT_CONFIG
#
#######################################################################
RAT_CONFIG=$(strip $(MTK_PROTOCOL1_RAT_CONFIG))

# As only chips after Jade,Everest,Olympus could apply auto config,
# the checking of MTK_ECCCI_C2K could skip, ie. default yes
# As result, what required is only to config MTK_PROTOCOL1_RAT_CONFIG
# in {project}.mk
RAT_CONFIG_ECCCI_C2K=yes

ifneq (,$(RAT_CONFIG))
  ifneq (,$(findstring C,$(RAT_CONFIG)))
    #C2K is supported
    #$(warning C2K is supported)
    RAT_CONFIG_C2K_SUPPORT=yes
    RAT_CONFIG_MD3_SUPPORT=2
    ifeq ($(RAT_CONFIG),C/W/G)
      RAT_CONFIG_IRAT_SUPPORT=no
    else
      RAT_CONFIG_IRAT_SUPPORT=yes
    endif
    ifeq ($(strip $(RAT_CONFIG_ECCCI_C2K)),yes)
      RAT_CONFIG_C2K_LTE_MODE=2
    else
      RAT_CONFIG_C2K_LTE_MODE=1
    endif
  else
    RAT_CONFIG_MD3_SUPPORT=0
    RAT_CONFIG_C2K_LTE_MODE=0
    RAT_CONFIG_IRAT_SUPPORT=no
  endif
  ifneq (,$(findstring L,$(RAT_CONFIG)))
    #LTE is supported
    #$(warning LTE is supported)
    RAT_CONFIG_LTE_SUPPORT=yes
  endif
  ifneq (,$(findstring T,$(RAT_CONFIG)))
    #TDS is supported
    RAT_CONFIG_TDS_SUPPORT=yes
  endif
  ifneq (,$(findstring W,$(RAT_CONFIG)))
    #TDS is supported
    RAT_CONFIG_WCDMA_SUPPORT=yes
  endif
  ifeq ($(strip $(RAT_CONFIG_ECCCI_C2K)),yes)
    ifeq ($(strip $(RAT_CONFIG)),Lf/Lt/T/G)
      RAT_CONFIG_MD1_SUPPORT=8
    else ifeq ($(RAT_CONFIG),Lf/Lt/W/G)
      RAT_CONFIG_MD1_SUPPORT=9
    else ifeq ($(RAT_CONFIG),Lf/Lt/W/T/G)
      RAT_CONFIG_MD1_SUPPORT=10
    else ifeq ($(RAT_CONFIG),W/T/G)
      RAT_CONFIG_MD1_SUPPORT=10
    else ifeq ($(RAT_CONFIG),C/Lf/Lt/W/G)
      RAT_CONFIG_MD1_SUPPORT=11
    else ifeq ($(RAT_CONFIG),C/Lf/Lt/W/T/G)
      RAT_CONFIG_MD1_SUPPORT=12
    else ifeq ($(RAT_CONFIG),Lt/T/G)
      RAT_CONFIG_MD1_SUPPORT=13
    else ifeq ($(RAT_CONFIG),Lf/W/G)
      RAT_CONFIG_MD1_SUPPORT=14
    else ifeq ($(RAT_CONFIG),C/Lf/W/G)
      RAT_CONFIG_MD1_SUPPORT=15
    else ifeq ($(RAT_CONFIG),C/Lf/Lt/T/G)
      RAT_CONFIG_MD1_SUPPORT=16
    else ifeq ($(RAT_CONFIG),C/Lt/T/G)
      RAT_CONFIG_MD1_SUPPORT=17
    else ifeq ($(RAT_CONFIG),C/W/G)
      RAT_CONFIG_MD1_SUPPORT=11
    else ifeq ($(RAT_CONFIG),W/G)
      RAT_CONFIG_MD1_SUPPORT=9
    else ifeq ($(RAT_CONFIG),C/T/G)
      RAT_CONFIG_MD1_SUPPORT=17
    else ifeq ($(RAT_CONFIG),T/G)
      RAT_CONFIG_MD1_SUPPORT=13
    else ifeq ($(RAT_CONFIG),G)
      RAT_CONFIG_MD1_SUPPORT=9
    else ifeq ($(RAT_CONFIG),C/W/T/G)
      RAT_CONFIG_MD1_SUPPORT=12
    else ifeq ($(RAT_CONFIG),C/Lf/Lt/G)
      RAT_CONFIG_MD1_SUPPORT=11
    else ifeq ($(RAT_CONFIG),Lf)
      RAT_CONFIG_MD1_SUPPORT=14
    else ifeq ($(RAT_CONFIG),Lf/Lt)
      RAT_CONFIG_MD1_SUPPORT=9
    else ifeq ($(RAT_CONFIG),C/Lf)
      RAT_CONFIG_MD1_SUPPORT=15
    else
      $(info MTK_PROTOCOL1_RAT_CONFIG - $(RAT_CONFIG))
      RAT_CONFIG_MD1_SUPPORT=0
      RAT_CONFIG_MD3_SUPPORT=0
      RAT_CONFIG_C2K_LTE_MODE=0
      RAT_CONFIG_IRAT_SUPPORT=no
      RAT_CONFIG_C2K_SUPPORT=no
      RAT_CONFIG_LTE_SUPPORT=no
    endif
  endif
else #if RAT_CONFIG is not set
  RAT_CONFIG_MD1_SUPPORT=0
  RAT_CONFIG_MD3_SUPPORT=0
  RAT_CONFIG_C2K_LTE_MODE=0
  RAT_CONFIG_IRAT_SUPPORT=no
  RAT_CONFIG_C2K_SUPPORT=no
  RAT_CONFIG_LTE_SUPPORT=no
endif

#autoset the value if not ever set before
ifeq (,$(strip $(MTK_MD1_SUPPORT)))
  MTK_MD1_SUPPORT=$(strip $(RAT_CONFIG_MD1_SUPPORT))
endif

ifeq (,$(strip $(MTK_MD3_SUPPORT)))
  MTK_MD3_SUPPORT=$(strip $(RAT_CONFIG_MD3_SUPPORT))
endif

ifeq (,$(strip $(MTK_C2K_LTE_MODE)))
  MTK_C2K_LTE_MODE=$(strip $(RAT_CONFIG_C2K_LTE_MODE))
endif

ifeq (,$(strip $(MTK_IRAT_SUPPORT)))
  MTK_IRAT_SUPPORT=$(strip $(RAT_CONFIG_IRAT_SUPPORT))
endif

ifneq ($(MTK_MD1_SUPPORT),)
  DEFINES += MTK_MD1_SUPPORT=$(strip $(MTK_MD1_SUPPORT))
endif

ifneq ($(MTK_MD3_SUPPORT),)
  DEFINES += MTK_MD3_SUPPORT=$(strip $(MTK_MD3_SUPPORT))
endif

ifneq ($(MTK_C2K_LTE_MODE),)
  DEFINES += MTK_C2K_LTE_MODE=$(strip $(MTK_C2K_LTE_MODE))
endif

ifeq ($(MTK_IRAT_SUPPORT),yes)
  DEFINES += MTK_IRAT_SUPPORT
endif

ifeq ($(strip $(RAT_CONFIG_LTE_SUPPORT)),yes)
  DEFINES += RAT_CONFIG_LTE_SUPPORT
endif

#$(error MD1/MD3/S/I/L is $(MTK_MD1_SUPPORT),$(MTK_MD3_SUPPORT),$(MTK_C2K_LTE_MODE),$(MTK_IRAT_SUPPORT),$(RAT_CONFIG_LTE_SUPPORT))
