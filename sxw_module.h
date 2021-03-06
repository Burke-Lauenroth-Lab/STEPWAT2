/********************************************************/
/********************************************************/
/*  Source file: sxw_module.h
 *  Type: header
 *  Application: STEPWAT - plant community dynamics simulator
 *               coupled with the  SOILWAT model.
 *  Purpose: Contains declarations relevant for the SXW_
 *           "module" made up of several source files.
 *  Applies to: sxw.c sxw_steppe.c sxw_soilwat.c */
/*  History:
 *     (22-May-2002) -- INITIAL CODING - cwb */
/********************************************************/
/********************************************************/

#ifndef SXW_MODULE_DEF
#define SXW_MODULE_DEF

#include "sw_src/SW_Control.h"
#include "sw_src/SW_Model.h"
#include "sw_src/SW_VegProd.h"
#include "sw_src/SW_Site.h"
#include "sw_src/SW_SoilWater.h"
#include "sw_src/SW_Files.h"

/* some macros for the production conversion array */
#define PC_Bmass 0
#define PC_Litter 1
#define PC_Live 2


/* These functions are found in sxw_resource.c */
void _sxw_root_phen(void);
void _sxw_update_resource(void);
void _sxw_update_root_tables( RealF sizes[] );


/* These functions are found in sxw_soilwat.c */
void  _sxw_sw_setup(RealF sizes[]);
void  _sxw_sw_run(void);
void  _sxw_sw_clear_transp(void);

/* These functions are found in sxw_environs.c */
void _sxw_set_environs(void);

/* testing code-- see sxw_tester.c */
void _sxw_test(void);

//sql
void connect(char *debugout);
void createTables(void);
void disconnect(void);
void insertInfo(void);
void insertRootsXphen(double * _rootsXphen);
void insertSXWPhen(void);
void insertSXWProd(void);
void insertInputVars(void);
void insertInputProd(void);
void insertInputSoils(void);
void insertOutputVars(RealF * _resource_cur, RealF added_transp);
void insertRgroupInfo(RealF * _resource_cur);
void insertOutputProd(SW_VEGPROD *v);
void insertRootsSum(RealD * _roots_active_sum);
void insertRootsRelative(RealD * _roots_active_rel);
void insertTranspiration(void);
void insertSWCBulk(void);


#endif
