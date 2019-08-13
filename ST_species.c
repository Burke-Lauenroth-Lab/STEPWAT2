/********************************************************/
/********************************************************/
/*  Source file: species.c
 *  Type: module
 *  Application: STEPPE - plant community dynamics simulator
 *  Purpose: Read / write and otherwise manage the
 *           site specific information.  See also the
 *           Layer module. */
 /*  History:
  *     (6/15/2000) -- INITIAL CODING - cwb */
 /********************************************************/
/********************************************************/

/* =================================================== */
/*                INCLUDES / DEFINES                   */
/* --------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include "ST_steppe.h"
#include "ST_globals.h"
#include "sw_src/filefuncs.h"
#include "myMemory.h"
#include "rands.h"
#include "sw_src/pcg/pcg_basic.h"

extern
  pcg32_random_t species_rng;

/******** Modular External Function Declarations ***********/
/* -- truly global functions are declared in functions.h --*/
/***********************************************************/
void rgroup_AddSpecies(GrpIndex rg, SppIndex sp);
void rgroup_DropSpecies(SppIndex sp);
Bool indiv_New(SppIndex sp);
void indiv_Kill_Complete(IndivType *ndv, int killType);
void indiv_proportion_Kill(IndivType *ndv, int killType, RealF proportionKilled);
void indiv_proportion_Recovery(IndivType *ndv, int killType,
		RealF proportionRecovery, RealF proportionKilled);
void indiv_proportion_Grazing(IndivType *ndv, RealF proportionGrazing);

void _delete(IndivType *ndv);
void save_annual_species_relsize(void);

/*------------------------------------------------------*/
/* Modular functions only used on one or two specific   */
/* places; that is, they are not generally useful       */
/* but have to be declared.                             */
void species_Update_Kills(SppIndex sp, IntS age);
void species_Update_Estabs(SppIndex sp, IntS num);
SppIndex species_New(void);

/*********** Locally Used Function Declarations ************/
/***********************************************************/
static SpeciesType *_create(void);

/****************** Begin Function Code ********************/
/***********************************************************/

IntS Species_NumEstablish(SppIndex sp)
{
	/*======================================================*/
	/* PURPOSE */
	/* return a number: 0 or more seedlings that establish */
	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000 */
	/*------------------------------------------------------*/

	//special conditions if we're using the grid and seed dispersal options (as long as its not during the spinup, because we dont use seed dispersal during spinup)
	if (UseGrid && UseSeedDispersal && !DuringSpinup) {
		if (Species[sp]->sd_sgerm)
		{
			if (Species[sp]->max_seed_estab <= 1) {
				return 1;
			} else {
				return (IntS) RandUniIntRange(1, Species[sp]->max_seed_estab, &species_rng);
			}
		} else {
			return 0;
		}
	}

	//float biomass = Species[sp]->relsize * Species[sp]->mature_biomass; //This line does nothing!
	if (RGroup[Species[sp]->res_grp]->est_annually
			|| LE(RandUni(&species_rng), Species[sp]->seedling_estab_prob)
			|| (Species[sp]->sd_sgerm)) {
		if (Species[sp]->max_seed_estab <= 1) {
			return 1;
		} else {
			return (IntS) RandUniIntRange(1, Species[sp]->max_seed_estab, &species_rng);
		    //return Species[sp]->max_seed_estab;
		}
	} else {
		return 0;
	}
}

/**************************************************************/
RealF Species_GetBiomass(SppIndex sp) {
	/*======================================================*/
	/* PURPOSE */
    /* Retrieve species biomass */
	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000            */
	/*------------------------------------------------------*/
	
	if (Species[sp]->est_count == 0) return 0.0;
	return (getSpeciesRelsize(sp) * Species[sp]->mature_biomass);
}

/**************************************************************/
void Species_Add_Indiv(SppIndex sp, Int new_indivs)
{
	/*======================================================*/
	/* PURPOSE */
	/* Add n=new_indivs individuals to the established list for
	 * species=sp.  add the species to the established list for
	 * the species group if needed and update the relsizes.   */

	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000            */

	/*------------------------------------------------------*/

	Int i;
	GrpIndex rg;
	RealF newsize = 0.0; /*accumulate total relsize for new indivs*/

	if (0 == new_indivs)
		return;

	rg = Species[sp]->res_grp;

	//printf("Inside Species_Add_Indiv() spIndex=%d, new_indivs=%d \n ",sp,  new_indivs);

	/* add individuals until max indivs */
	for (i = 1; i <= new_indivs; i++)
	{
		if (!indiv_New(sp))
		{
			LogError(logfp, LOGFATAL, "Unable to add new individual in Species_Add_Indiv()");
		}

		Species[sp]->est_count++;
		newsize += Species[sp]->relseedlingsize;
		//printf("Loop Index i=%d, Species[sp]->relseedlingsize=%.5f now newsize=%.5f \n ",i,  Species[sp]->relseedlingsize,newsize);
	}

	/* add species to species group if new*/
	rgroup_AddSpecies(rg, sp);

	//printf("Inside Species_Add_Indiv() calculated total newsize=%.5f \n ",newsize);
}

/**************************************************************/
void species_Update_Kills(SppIndex sp, IntS age)
{
	/*======================================================*/
	/* PURPOSE */
	/* accumulate frequencies of kills by age for survivorship */
	/* 'kills' is a pointer to a dynamically sized array created
	 * in params_check_species().
	 *
	 * Important note: age is base1 so it must be decremented
	 * for the base0 arrays.
	 */

	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 5/18/2001            */

	/*------------------------------------------------------*/

	if (!isnull(Species[sp]->kills))
	{
		age--;
		if (age >= Species[sp]->max_age) //a quick check to keep from writing off the end of the kills arrays (which was happening in really obscure cases)...
			return;
		Species[sp]->kills[age]++;
		RGroup[Species[sp]->res_grp]->kills[age]++;
	}
}

/**************************************************************/
void species_Update_Estabs(SppIndex sp, IntS num)
{
	/*======================================================*/
	/* PURPOSE */
	/* accumulate number of indivs established by species */
	/* for fecundity rates.                               */

	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 5/21/2001            */

	/*------------------------------------------------------*/

	Species[sp]->estabs += num;
	RGroup[Species[sp]->res_grp]->estabs += num;
}

/** \brief Returns the relsize of Species[sp]
 * 
 * \param sp the index of the Species array that you want to measure.
 * 
 * \return RealF greater than of equal to 0 representing the summed
 *         relsizes of all individuals in Species[sp]
 * 
 * \sa getRGroupRelsize()
 */
RealF getSpeciesRelsize(SppIndex sp)
{
	IndivType *p = Species[sp]->IndvHead;
    double sum = 0;

	if(p)
	{
    	while(p)
    	{
        	sum += p->relsize;
			p = p->Next;
    	}
	}

	return (RealF) (sum + Species[sp]->extragrowth);
}

/**
 * \brief Create a new species and integrate it into Species.
 * 
 * \return index of the new species inside Species.
 * 
 * Allocates memory of the new species and integrates it into
 * Species.
 * 
 * \sa Species
 * \sa _create()
 */
SppIndex species_New(void)
{
	/*======================================================*/
	/* PURPOSE */
	/* Create a new species object and give it the next
	 * consecutive identifier.
	 *
	 * Return the index of the new object.
	 *
	 * Initialization is performed in parm_Species_Init
	 * but the list of individuals in the species is
	 * maintained by a linked list which is of course
	 * empty when the species is first created.  See the
	 * Indiv module for details. */

	 /* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000            */

	/*------------------------------------------------------*/
	SppIndex i = (SppIndex) Globals.sppCount;

	if (++Globals.sppCount > MAX_SPECIES)
	{
		LogError(logfp, LOGFATAL, "Too many species specified (>%d)!\n"
				"You must adjust MAX_SPECIES and recompile!\n",
		MAX_SPECIES);
	}

	Species[i] = _create();
	Species[i]->IndvHead = NULL;
	return i;
}

/**
 * \brief Allocates memory for a new SpeciesType.
 * 
 * \return pointer to the newely allocated struct.
 * 
 * Used in species_New.
 * 
 * \sa species_New()
 */
static SpeciesType *_create(void)
{
	/*======================================================*/
	/* PURPOSE */

	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000            */

	/*------------------------------------------------------*/

	SpeciesType *p;

	p = (SpeciesType *) Mem_Calloc(1, sizeof(SpeciesType), "Species_Create");
        p->name = Mem_Calloc(Globals.max_speciesnamelen + 1, sizeof(char), "Species_Create");

	return (p);

}

/**************************************************************/
SppIndex Species_Name2Index(const char *name)
{
	/*======================================================*/
	/* PURPOSE */

	/* HISTORY */
	/* Chris Bennett @ LTER-CSU 6/15/2000            */

	/*------------------------------------------------------*/
	Int i, sp = -1;
	ForEachSpecies(i)
	{
		if (strcmp(name, Species[i]->name) == 0)
		{
			sp = i;
			break;
		}
	}
	return ((SppIndex) sp);
}

void Species_Annual_Kill(const SppIndex sp, int killType)
{
	/*======================================================*/
	/* PURPOSE */
	/* To kill all the annual species and their individuals */
	/* HISTORY */
	/* Added - Nov 4th 2015 -AT */
	/*------------------------------------------------------*/

	IndivType *p1 = Species[sp]->IndvHead, *t1;
	while (p1)
	{
		t1 = p1->Next;
		_delete(p1);
		p1 = t1;
	}
	rgroup_DropSpecies(sp);
}

void Species_Proportion_Kill(const SppIndex sp, int killType,
		RealF proportionKilled)
{
	/*======================================================*/
	/* PURPOSE */
	/* Proportion Killed all established individuals in a species.
	 *
	 * Note the special loop construct.  we have to save the
	 * pointer to next prior to killing because the object
	 * is deleted. */

	 /* HISTORY */
	/* Chris Bennett @ LTER-CSU 11/15/2000            */
	/*   8/3/01 - cwb - added linked list processing.
	 *   09/23/15 -AT  -Added proportionKilled for all even for annual
	 *   now deletion of species and individual is hold till recovery function */
	/*------------------------------------------------------*/

	IndivType *p = Species[sp]->IndvHead, *t;
	//kill  all the species individuals  proportionally or adjust their real size irrespective of being annual or perennial, both will have this effect
	while (p)
	{
		t = p->Next;
		indiv_proportion_Kill(p, killType, proportionKilled);
		p = t;
	}
}

void Species_Proportion_Grazing(const SppIndex sp, RealF proportionGrazing)
{
	/*======================================================*/
	/* PURPOSE */
	/* Proportion Grazing on all individuals and on the extra growth that 
         * resulted from extra resources this year,stored in Species->extragrowth. */
	 /* HISTORY */
	/* AT  1st Nov 2015 -Added Species Proportion Grazing for all even for annual */
	/* 14 August 2018 -CH -Added functionality to graze the species' extra growth. */
	/*------------------------------------------------------*/

	//CH- extra growth is only stored at the species level. This will graze extra
	//    growth for the whole species.
	//    loss represents the proportion of extragrowth that is eaten by livestock
	RealF loss = Species[sp]->extragrowth * proportionGrazing;

	//CH- Remove the loss from Species extra growth.
	Species[sp]->extragrowth -= loss;	// remove the loss from extragrowth

	//Implement grazing on normal growth for all individuals in each species.
	IndivType *t, *p = Species[sp]->IndvHead;
	while (p) //while p points to an individual
	{
		t = p->Next; //must store Next since p might be deleted at any time.
		indiv_proportion_Grazing(p, proportionGrazing);
		p = t; //move to the next plant.
	}
}

void Species_Proportion_Recovery(const SppIndex sp, int killType,
        RealF proportionRecovery, RealF proportionKilled) {
    /*======================================================*/
    /* PURPOSE */
    /* Proportion Recovery (representing re-sprouting after fire) for all 
     * established individuals in a species. Note the special loop construct.  
     * We have to save the pointer to next prior to killing because the object
     * is deleted. */
    /* HISTORY */
    /* AT  1st Nov 2015 -Added Species Proportion Recovery  */
    /*------------------------------------------------------*/
    
    IndivType *p = Species[sp]->IndvHead, *t;
    //Recover biomass for each perennial species that is established
    while (p) {
        t = p->Next;
        indiv_proportion_Recovery(p, killType, proportionRecovery,
                proportionKilled);
        p = t;
    }
    //printf("'within proportion_recovery after first killing': Species = %s, relsize = %f, est_count = %d\n",Species[sp]->name, Species[sp]->relsize, Species[sp]->est_count);
   
}

/**************************************************************/
void Species_Kill(const SppIndex sp, int killType)
{
    /*======================================================*/
    /* PURPOSE */
    /* Kill all established individuals in a species.
     *
     * Note the special loop construct.  we have to save the
     * pointer to next prior to killing because the object
     * is deleted. */
     /* HISTORY */
    /* Chris Bennett @ LTER-CSU 11/15/2000 */
    /* 8/3/01 - cwb - added linked list processing. */
    /*------------------------------------------------------*/
    
    IndivType *p = Species[sp]->IndvHead, *t;

    while (p)
    {
        t = p->Next;
        indiv_Kill_Complete(p, killType);
        p = t;
    }
    
    rgroup_DropSpecies(sp);

}
void save_annual_species_relsize() {
    int sp = 0;

    ForEachSpecies(sp) {
        if (Species[sp]->max_age == 1) {
            //printf("Globals.currYear = %d, sp=%d , Species[sp]->relsize=%.5f ,old value lastyear_relsize : %.5f \n", Globals.currYear, sp, Species[sp]->relsize, Species[sp]->lastyear_relsize);
            Species[sp]->lastyear_relsize = getSpeciesRelsize(sp);
            //Species[sp]->lastyear_relsize = 2;
            //printf("Globals.currYear = %d, sp=%d new updated value lastyear_relsize : %.5f \n", Globals.currYear, sp, Species[sp]->lastyear_relsize);
        }
    }
}

#ifdef DEBUG_MEM
#include "myMemory.h"
/*======================================================*/
void Species_SetMemoryRefs( void)
{
	/* when debugging memory problems, use the bookkeeping
	 code in myMemory.c
	 This routine sets the known memory refs in this module
	 so they can be  checked for leaks, etc.  All refs will
	 have been cleared by a call to ClearMemoryRefs() before
	 this, and will be checked via CheckMemoryRefs() after
	 this, most likely in the main() function.

	 EVERY dynamic allocation must be noted here or the
	 check will fail (which is the point, to catch unknown
	 or missing pointers to memory).

	 */
	SppIndex sp;
	IndivType *p;

	ForEachSpecies(sp)
	{
		NoteMemoryRef(Species[sp]);
		NoteMemoryRef(Species[sp]->kills);
		p = Species[sp]->IndvHead;
		while (p)
		{
			NoteMemoryRef(p);
			p = p->Next;
		}
	}

}

#endif
