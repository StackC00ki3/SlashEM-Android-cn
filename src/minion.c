/*	SCCS Id: @(#)minion.c	3.4	2003/01/09	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "emin.h"
#include "epri.h"

void
msummon(mon)		/* mon summons a monster */
struct monst *mon;
{
	register struct permonst *ptr;
	register int dtype = NON_PM, cnt = 0;
	aligntyp atyp;
	struct monst *mtmp;

	if (mon) {
	    ptr = mon->data;
	    atyp = (ptr->maligntyp==A_NONE) ? A_NONE : sgn(ptr->maligntyp);
	    if (mon->ispriest || mon->data == &mons[PM_ALIGNED_PRIEST]
		|| mon->data == &mons[PM_ANGEL])
		atyp = EPRI(mon)->shralign;
	} else {
	    ptr = &mons[PM_WIZARD_OF_YENDOR];
	    atyp = (ptr->maligntyp==A_NONE) ? A_NONE : sgn(ptr->maligntyp);
	}
	    
	if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {
	    dtype = (!rn2(20)) ? dprince(atyp) :
				 (!rn2(4)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_dlord(ptr)) {
	    dtype = (!rn2(50)) ? dprince(atyp) :
				 (!rn2(20)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_ndemon(ptr)) {
	    dtype = (!rn2(20)) ? dlord(atyp) :
				 (!rn2(6)) ? ndemon(atyp) : monsndx(ptr);
	    cnt = 1;
	} else if (is_lminion(mon)) {
	    dtype = (is_lord(ptr) && !rn2(20)) ? llord() :
		     (is_lord(ptr) || !rn2(6)) ? lminion() : monsndx(ptr);
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	} else if (ptr == &mons[PM_ANGEL]) {
	    /* non-lawful angels can also summon */
	    if (!rn2(6)) {
		switch (atyp) { /* see summon_minion */
		case A_NEUTRAL:
		    dtype = PM_AIR_ELEMENTAL + rn2(4);
		    break;
		case A_CHAOTIC:
		case A_NONE:
		    dtype = ndemon(atyp);
		    break;
		}
	    } else {
		dtype = PM_ANGEL;
	    }
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	}

	if (dtype == NON_PM) return;

	/* sanity checks */
	if (cnt > 1 && (mons[dtype].geno & G_UNIQ)) cnt = 1;
	/*
	 * If this daemon is unique and being re-summoned (the only way we
	 * could get this far with an extinct dtype), try another.
	 */
	if (mvitals[dtype].mvflags & G_GONE) {
	    dtype = ndemon(atyp);
	    if (dtype == NON_PM) return;
	}

	while (cnt > 0) {
	    mtmp = makemon(&mons[dtype], u.ux, u.uy, NO_MM_FLAGS);
	    if (mtmp && (dtype == PM_ANGEL)) {
		/* alignment should match the summoner */
		EPRI(mtmp)->shralign = atyp;
	    }
	    cnt--;
	}
}

void
summon_minion(alignment, talk)
aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int)alignment) {
	case A_LAWFUL:
	    mnum = lminion();
	    break;
	case A_NEUTRAL:
	    mnum = PM_AIR_ELEMENTAL + rn2(4);
	    break;
	case A_CHAOTIC:
	case A_NONE:
	    mnum = ndemon(alignment);
	    break;
	default:
	    impossible("啊？你没阵营的吗大哥？");
	    mnum = ndemon(A_NONE);
	    break;
    }
    if (mnum == NON_PM) {
	mon = 0;
    } else if (mons[mnum].pxlth == 0) {
	struct permonst *pm = &mons[mnum];
	mon = makemon(pm, u.ux, u.uy, MM_EMIN);
	if (mon) {
	    mon->isminion = TRUE;
	    EMIN(mon)->min_align = alignment;
	}
    } else if (mnum == PM_ANGEL) {
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
	if (mon) {
	    mon->isminion = TRUE;
	    EPRI(mon)->shralign = alignment;	/* always A_LAWFUL here */
	}
    } else
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    if (mon) {
	if (talk) {
	    pline_The("%s的声音如霹雳般响起：", align_gname(alignment));
	    verbalize("汝将为汝之莽行付出代价！");
	    if (!Blind)
		pline("%s出现在你面前。", Amonnam(mon));
	}
	mon->mpeaceful = FALSE;
	/* don't call set_malign(); player was naughty */
    }
}
#define Athome	(Inhell && !mtmp->cham)

int
demon_talk(mtmp)		/* returns 1 if it won't attack. */
register struct monst *mtmp;
{
	long cash, demand, offer;

	if (uwep && uwep->oartifact == ART_EXCALIBUR) {
	    pline("%s看起来非常生气。", Amonnam(mtmp));
	    mtmp->mpeaceful = mtmp->mtame = 0;
	    set_malign(mtmp);
	    newsym(mtmp->mx, mtmp->my);
	    return 0;
	}

	/* Slight advantage given. */
	if (is_dprince(mtmp->data) && mtmp->minvis) {
	    mtmp->minvis = mtmp->perminvis = 0;
	    if (!Blind) pline("%s出现在你面前。", Amonnam(mtmp));
	    newsym(mtmp->mx,mtmp->my);
	}
	if (youmonst.data->mlet == S_DEMON) {	/* Won't blackmail their own. */
	    pline("%s向你打招呼：“祝你狩猎快乐啊，%s。”",
		  Amonnam(mtmp), flags.female ? "老妹" : "老兄");
	    if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
	    return(1);
	}
#ifndef GOLDOBJ
	cash = u.ugold;
#else
	cash = money_cnt(invent);
#endif
	demand = (cash * (rnd(80) + 20 * Athome)) /
	    (100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))));

	if (!demand) {		/* you have no gold */
	    mtmp->mpeaceful = 0;
	    set_malign(mtmp);
	    return 0;
	} else {
	    /* make sure that the demand is unmeetable if the monster
	       has the Amulet, preventing monster from being satisified
	       and removed from the game (along with said Amulet...) */
	    if (mon_has_amulet(mtmp))
		demand = cash + (long)rn1(1000,40);

	    pline("%s要求你支付%ld%s的买路钱！",
		  Amonnam(mtmp), demand, currency(demand));

	    if ((offer = bribe(mtmp)) >= demand) {
		pline("%s狠狠地嘲笑了你的怂逼行为，然后消失不见了。",
		      Amonnam(mtmp));
	    } else if (offer > 0L && (long)rnd(40) > (demand - offer)) {
		pline("%s垮着个脸瞪了你几眼，然后消失了。",
		      Amonnam(mtmp));
	    } else {
		pline("%s生气了……", Amonnam(mtmp));
		mtmp->mpeaceful = 0;
		set_malign(mtmp);
		return 0;
	    }
	}
	mongone(mtmp);
	return(1);
}

int lawful_minion(int difficulty)
/* this routine returns the # of an appropriate minion,
   given a difficulty rating from 1 to 30 */
{
   difficulty = difficulty + rn2(5) - 2;
   if (difficulty < 0) difficulty = 0;
   if (difficulty > 30) difficulty = 30;
   difficulty /= 3;
   switch (difficulty) {
      case 0: return PM_TENGU;
      case 1: return PM_COUATL;
      case 2: return PM_WHITE_UNICORN;
      case 3: return PM_MOVANIC_DEVA;
      case 4: return PM_MONADIC_DEVA;
      case 5: return PM_KI_RIN;
      case 6: return PM_ASTRAL_DEVA;
      case 7: return PM_ARCHON;
      case 8: return PM_PLANETAR;
      case 9: return PM_SOLAR;
      case 10: return PM_SOLAR;

      default: return PM_TENGU;
   }
}

int neutral_minion(int difficulty)
/* this routine returns the # of an appropriate minion,
   given a difficulty rating from 1 to 30 */
{
   difficulty = difficulty + rn2(9) - 4;
   if (difficulty < 0) difficulty = 0;
   if (difficulty > 30) difficulty = 30;
   if (difficulty < 6) return PM_GRAY_UNICORN;
   if (difficulty < 15) return (PM_AIR_ELEMENTAL+rn2(4));
   return (PM_DJINNI /* +rn2(4) */);
}

int chaotic_minion(int difficulty)
/* this routine returns the # of an appropriate minion,
   given a difficulty rating from 1 to 30 */
{
   difficulty = difficulty + rn2(5) - 2;
   if (difficulty < 0) difficulty = 0;
   if (difficulty > 30) difficulty = 30;
   /* KMH, balance patch -- avoid using floating-point (not supported by all ports) */
/*   difficulty = (int)((float)difficulty / 1.5);*/
   difficulty = (difficulty * 2) / 3;
   switch (difficulty) {
      case 0: return PM_GREMLIN;
      case 1:
      case 2: return (PM_DRETCH+rn2(5));
      case 3: return PM_BLACK_UNICORN;
      case 4: return PM_BLOOD_IMP;
      case 5: return PM_SPINED_DEVIL;
      case 6: return PM_SHADOW_WOLF;
      case 7: return PM_HELL_HOUND;
      case 8: return PM_HORNED_DEVIL;
      case 9: return PM_BEARDED_DEVIL;
      case 10: return PM_BAR_LGURA;
      case 11: return PM_CHASME;
      case 12: return PM_BARBED_DEVIL;
      case 13: return PM_VROCK;
      case 14: return PM_BABAU;
      case 15: return PM_NALFESHNEE;
      case 16: return PM_MARILITH;
      case 17: return PM_NABASSU;
      case 18: return PM_BONE_DEVIL;
      case 19: return PM_ICE_DEVIL;
      case 20: return PM_PIT_FIEND;
   }
   return PM_GREMLIN;
}

long
bribe(mtmp)
struct monst *mtmp;
{
	char buf[BUFSZ];
	long offer;
#ifdef GOLDOBJ
	long umoney = money_cnt(invent);
#endif

	getlin("你准备付多少钱？", buf);
	if (sscanf(buf, "%ld", &offer) != 1) offer = 0L;

	/*Michael Paddon -- fix for negative offer to monster*/
	/*JAR880815 - */
	if (offer < 0L) {
		You("试图糊弄%s，然后让他反过来给你钱……不过没成功。",
			mon_nam(mtmp));
		return 0L;
	} else if (offer == 0L) {
		You("叫他滚蛋。");
		return 0L;
#ifndef GOLDOBJ
	} else if (offer >= u.ugold) {
		You("把你所有的金币都塞给了%s。", mon_nam(mtmp));
		offer = u.ugold;
	} else {
		You("向%s付了%ld%s。", mon_nam(mtmp), offer, currency(offer));
	}
	u.ugold -= offer;
	mtmp->mgold += offer;
#else
	} else if (offer >= umoney) {
		You("把你所有的钱都塞给了%s。", mon_nam(mtmp));
		offer = umoney;
	} else {
		You("向%s付了%ld%s。", mon_nam(mtmp), offer, currency(offer));
	}
	(void) money2mon(mtmp, offer);
#endif
	flags.botl = 1;
	return(offer);
}

int
dprince(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(dlord(atyp));	/* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(ndemon(atyp));	/* approximate */
}

/* create lawful (good) lord */
int
llord()
{
	if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
		return(PM_ARCHON);

	return(lminion());	/* approximate */
}

int
lminion()
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_ANGEL,0);
	    if (ptr && !is_lord(ptr))
		return(monsndx(ptr));
	}

	return NON_PM;
}

int
ndemon(atyp)
aligntyp atyp;
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_DEMON, 0);
	    if (ptr && is_ndemon(ptr) &&
		    (atyp == A_NONE || sgn(ptr->maligntyp) == sgn(atyp)))
		return(monsndx(ptr));
	}

	return NON_PM;
}

/*minion.c*/
