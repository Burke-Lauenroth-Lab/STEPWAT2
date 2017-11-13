/********************************************************/
/********************************************************/
/*  Source file: sxw.h
/*  Type: header
/*  Purpose: Contains pertinent declarations, global variables,
 *           etc required to support new functions that
 *           interface STEPPE with SOILWAT.
/*  Application: STEPWAT - plant community dynamics simulator
 *               coupled with the  SOILWAT model.
/*  History:
/*     (14-Apr-2002) -- INITIAL CODING - cwb
/*
/********************************************************/
/********************************************************/

#ifndef SXW_DEF
#define SXW_DEF

/* comment the next line to use STEPPE-provided values
 * of biomass and size when computing SOILWAT parameters
 * or scaling transpiration.  If defined, the macro
 * sets the sizes to the maximum.
 */
/*#define SXW_BYMAXSIZE*/


#include "generic.h"
#include "SW_Times.h"
#include "ST_defines.h"

int getNTranspLayers(int veg_prod_type);

struct stepwat_st {
  RealD *transpTotal; /* points to dynamic array indexed by Ilp() */
  RealD *transpTrees;
  RealD *transpShrubs;
  RealD *transpForbs;
  RealD *transpGrasses;

  RealD *transpTotal_avg,
        *transpTrees_avg,
        *transpShrubs_avg,
        *transpForbs_avg,
        *transpGrasses_avg;

  RealF  temp,   /* soilwat's MAT */
         ppt;    /* soilwat's MAP */
  TimeInt NPds;  /* number of transp periods= maxdays, maxweeks, maxmonths */
  IntUS NTrLyrs, /* # transp. layers taken from SOILWAT */
        NGrps;   /* # plant groups taken from STEPPE */
  IntUS NSoLyrs;  /* number of soil layers defined */

  /* These are file names */
  char  *f_files,  /* list of input files for sxw */
        *f_roots,  /* root distributions */
        *f_phen,   /* phenology */
        *f_bvt,    /* biomass vs transpiration 12/29/03 */
        *f_prod,   /* biomass to prod. conv. nos. */
        *f_watin;  /* soilwat's input file */

  /* DEBUG stuff */
  char *debugfile; /* added in ST_Main(), read to get debug instructions */
  RealF *swc, /* dynamic array(Ilp) of SWC from SOILWAT */
         aet;     /* soilwat's evapotranspiration for the year */
  RealD  surfaceTemp;   /* soilwat's surfaceTemp */

  // PPT variables
  int    yearInterval; // keep track of years
  int    curMonth;

  RealF PPT_sum,
        PPT_rain,
        PPT_snow_fall,
        PPT_snow_melt,
        PPT_snow_loss;

  RealF *SWAbulk_grass_avg, // 2D array to store SWA vals ([days of year][number of max layers])
        *SWAbulk_shrub_avg,
        *SWAbulk_tree_avg,
        *SWAbulk_forb_avg;

  RealF *SWA_master, // 4D array to store SWA for all veg_types
        *dSWAbulk, // store actual available SWA
        *dSWA_repartitioned; // store repartioned SWA values

  //int rank_SWPcrits[5]; // array to store the SWP crits in order of lest negative to most negative (used in sxw_resource)

  RealF transp_SWA[MAX_YEARS][11]; // store the sum of SWA and transp for each year and resource. transp_SWA[year][steppe_resource_group]
  int rank_SWPcrits[5]; // array to store the SWP crits in order of lest negative to most negative (used in sxw_resource)

  // used in SW_Output.c for creating column headers
  int col_status_dy;
  int col_status_wk;
  int col_status_mo;
  int col_status_yr;
};

#define SXW_NFILES 5



typedef struct stepwat_st SXW_t;

#define ForEachTrPeriod(i) for((i)=0; (i)< SXW.NPds; (i)++)


/* convert 3-d index to actual array index for
   group/layer/phenology 3d table */
#define Iglp(g,l,p) (((g)*SXW.NTrLyrs*SXW.NPds) + ((l)*SXW.NPds) + (p))

/* convert 3-d index to actual array index for
 * veg-prod-type/layer/phenology
 */
#define Itlp(t,l,p) (((t)*SXW.NTrLyrs*SXW.NPds) + ((l)*SXW.NPds) + (p))

/* convert 4-d index to actual array index for
 * veg-prod-type/crit-value/layer/phenology
 */
#define Itclp(t,c,l,p) (((t)*SXW.NTrLyrs*SXW.NPds) + ((c)*4) + ((l)*SXW.NPds) + (p)) // c*4 is because there are 4 critical values

/* convert 2d layer by period indices to
  layer/phenology 1D index */
#define Ilp(l,p) ((l)*SXW.NPds + (p))

/* convert 2d group by period indices to
   group/phenology 1D index */
#define Igp(g,p) ((g)*SXW.NPds + (p))

/* convert 2d group by layer indices to
   layer/period 1D index */
#define Ilg(l,g) ((l)*SXW.NGrps + (g))

#endif
