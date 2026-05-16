/*	SCCS Id: @(#)apply.c	3.4	2003/11/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static const char tools[] = { TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static const char tools_too[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS,
				  WEAPON_CLASS, WAND_CLASS, GEM_CLASS, 0 };
static const char tinnables[] = { ALLOW_FLOOROBJ, FOOD_CLASS, 0 };

#ifdef TOURIST
STATIC_DCL int FDECL(use_camera, (struct obj *));
#endif
STATIC_DCL int FDECL(use_towel, (struct obj *));
STATIC_DCL boolean FDECL(its_dead, (int,int,int *));
STATIC_DCL int FDECL(use_stethoscope, (struct obj *));
STATIC_DCL void FDECL(use_whistle, (struct obj *));
STATIC_DCL void FDECL(use_magic_whistle, (struct obj *));
STATIC_DCL void FDECL(use_leash, (struct obj *));
STATIC_DCL int FDECL(use_mirror, (struct obj *));
STATIC_DCL void FDECL(use_bell, (struct obj **));
STATIC_DCL void FDECL(use_candelabrum, (struct obj *));
STATIC_DCL void FDECL(use_candle, (struct obj **));
STATIC_DCL void FDECL(use_lamp, (struct obj *));
STATIC_DCL int FDECL(use_torch, (struct obj *));
STATIC_DCL void FDECL(light_cocktail, (struct obj *));
STATIC_DCL void FDECL(use_tinning_kit, (struct obj *));
STATIC_DCL void FDECL(use_figurine, (struct obj **));
STATIC_DCL void FDECL(use_grease, (struct obj *));
STATIC_DCL void FDECL(use_trap, (struct obj *));
STATIC_DCL void FDECL(use_stone, (struct obj *));
STATIC_PTR int NDECL(set_trap);		/* occupation callback */
STATIC_DCL int FDECL(use_whip, (struct obj *));
STATIC_DCL int FDECL(use_pole, (struct obj *));
STATIC_DCL int FDECL(use_cream_pie, (struct obj *));
STATIC_DCL int FDECL(use_grapple, (struct obj *));
STATIC_DCL int FDECL(do_break_wand, (struct obj *));
STATIC_DCL boolean FDECL(figurine_location_checks,
				(struct obj *, coord *, BOOLEAN_P));
STATIC_DCL boolean NDECL(uhave_graystone);
STATIC_DCL void FDECL(add_class, (char *, CHAR_P));

#ifdef	AMIGA
void FDECL( amii_speaker, ( struct obj *, char *, int ) );
#endif

const char no_elbow_room[] = "你的胳膊没有足够的挥动空间.";

#ifdef TOURIST
STATIC_OVL int
use_camera(obj)
	struct obj *obj;
{
	register struct monst *mtmp;

	if(Underwater) {
		pline("你的相机不是防水的，把它泡进水里会让你质保过期，而且很明显镜头会坏掉.");
		return(0);
	}
	if(!getdir((char *)0)) return(0);

	if (obj->spe <= 0) {
		pline(nothing_happens);
		return (1);
	}
	consume_obj_charge(obj, TRUE);

	if (obj->cursed && !rn2(2)) {
		(void) zapyourself(obj, TRUE);
	} else if (u.uswallow) {
		You("你给%s的%s拍了个照.", s_suffix(mon_nam(u.ustuck)),
		    mbodypart(u.ustuck, STOMACH));
	} else if (u.dz) {
		You("你给%s拍了个照片.",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
	} else if (!u.dx && !u.dy) {
		(void) zapyourself(obj, TRUE);
	} else if ((mtmp = bhit(u.dx,u.dy,COLNO,FLASHED_LIGHT,
				(int FDECL((*),(MONST_P,OBJ_P)))0,
				(int FDECL((*),(OBJ_P,OBJ_P)))0,
				&obj)) != 0) {
		obj->ox = u.ux,  obj->oy = u.uy;
		(void) flash_hits_mon(mtmp, obj);
	}
	return 1;
}
#endif

STATIC_OVL int
use_towel(obj)
	struct obj *obj;
{
	if(!freehand()) {
		You("你的%s空不出来!", body_part(HAND));
		return 0;
	} else if (obj->owornmask) {
		You("在穿戴着它的时候没办法使用它!");
		return 0;
	} else if (obj->cursed) {
		long old;
		switch (rn2(3)) {
		case 2:
		    old = Glib;
		    incr_itimeout(&Glib, rn1(10, 3));
		    Your("%s %s!", makeplural(body_part(HAND)),
			(old ? "比以前更加丰满了" : "变得滑滑腻腻的"));
		    return 1;
		case 1:
		    if (!ublindf) {
			old = u.ucreamed;
			u.ucreamed += rn1(10, 3);
			pline("啊! 你的%s %s油污！", body_part(FACE),
			      (old ? "黏上了更多" : "糊满了"));
			make_blinded(Blinded + (long)u.ucreamed - old, TRUE);
		    } else {
			const char *what = (ublindf->otyp == LENSES) ?
					    "眼镜" : "眼罩";
			if (ublindf->cursed) {
			    You("推了推你的%s %s.", what,
				rn2(2) ? "东倒西歪的" : "歪斜的");
			} else {
			    struct obj *saved_ublindf = ublindf;
			    You("把你的%s摘掉了.", what);
			    Blindf_off(ublindf);
			    dropx(saved_ublindf);
			}
		    }
		    return 1;
		case 0:
		    break;
		}
	}

	if (Glib) {
		Glib = 0;
		You("擦了擦你的%s.", makeplural(body_part(HAND)));
		return 1;
	} else if(u.ucreamed) {
		Blinded -= u.ucreamed;
		u.ucreamed = 0;

		if (!Blinded) {
			pline("你把脸上那坨奶油擦掉了.");
			Blinded = 1;
			make_blinded(0L,TRUE);
		} else {
			Your("%s现在感觉很清爽.", body_part(FACE));
		}
		return 1;
	}

	Your("%s以及你的%s已经很干净了.",
		body_part(FACE), makeplural(body_part(HAND)));

	return 0;
}

/* maybe give a stethoscope message based on floor objects */
STATIC_OVL boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
	struct obj *otmp;
	struct trap *ttmp;

	if (!can_reach_floor()) return FALSE;

	/* additional stethoscope messages from jyoung@apanix.apana.org.au */
	if (Hallucination && sobj_at(CORPSE, rx, ry)) {
	    /* (a corpse doesn't retain the monster's sex,
	       so we're forced to use generic pronoun here) */
	    You_hear("一个声音说道：\"吉姆，它死了！\"");
	    *resp = 1;
	    return TRUE;
	} else if (Role_if(PM_HEALER) && ((otmp = sobj_at(CORPSE, rx, ry)) != 0 ||
				    (otmp = sobj_at(STATUE, rx, ry)) != 0)) {
	    /* possibly should check uppermost {corpse,statue} in the pile
	       if both types are present, but it's not worth the effort */
	    if (vobj_at(rx, ry)->otyp == STATUE) otmp = vobj_at(rx, ry);
	    if (otmp->otyp == CORPSE) {
		You("确定%s倒霉的家伙已经死了.",
		    (rx == u.ux && ry == u.uy) ? "这个" : "那个");
	    } else {
		ttmp = t_at(rx, ry);
		pline("%s作为一个雕像来说，它的生理指标%s.",
		      The(mons[otmp->corpsenm].mname),
		      (ttmp && ttmp->ttyp == STATUE_TRAP) ?
			"非常好" : "很棒");
	    }
	    return TRUE;
	}
	return FALSE;
}

static const char hollow_str[] = "一个空洞的声音，这背后一定有个%s!";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
STATIC_OVL int
use_stethoscope(obj)
	register struct obj *obj;
{
	static long last_used_move = -1;
	static short last_used_movement = 0;
	struct monst *mtmp;
	struct rm *lev;
	int rx, ry, res;
	boolean interference = (u.uswallow && is_whirly(u.ustuck->data) &&
				!rn2(Role_if(PM_HEALER) ? 10 : 3));

	if (nohands(youmonst.data)) {	/* should also check for no ears and/or deaf */
		You("的手空不出来！");	/* not `body_part(HAND)' */
		return 0;
	} else if (!freehand()) {
		You("没有空余的%s.", body_part(HAND));
		return 0;
	}
	if (!getdir((char *)0)) return 0;

	res = (moves == last_used_move) &&
	      (youmonst.movement == last_used_movement);
	last_used_move = moves;
	last_used_movement = youmonst.movement;

#ifdef STEED
	if (u.usteed && u.dz > 0) {
		if (interference) {
			pline("%s挡住了你.", Monnam(u.ustuck));
			mstatusline(u.ustuck);
		} else
			mstatusline(u.usteed);
		return res;
	} else
#endif
	if (u.uswallow && (u.dx || u.dy || u.dz)) {
		mstatusline(u.ustuck);
		return res;
	} else if (u.uswallow && interference) {
		pline("%s挡住了你.", Monnam(u.ustuck));
		mstatusline(u.ustuck);
		return res;
	} else if (u.dz) {
		if (Underwater)
		    You_hear("轻微的的水声.");
		else if (u.dz < 0 || !can_reach_floor())
		    You_cant("碰到%s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		else if (its_dead(u.ux, u.uy, &res))
		    ;	/* message already given */
		else if (Is_stronghold(&u.uz))
		    You_hear("地狱中的火噼啪作响.");
		else
		    pline_The("%s很健康.", surface(u.ux,u.uy));
		return res;
	} else if (obj->cursed && !rn2(2)) {
		You_hear("你的心跳声.");
		return res;
	}
	if (Stunned || (Confusion && !rn2(5))) confdir();
	if (!u.dx && !u.dy) {
		ustatusline();
		return res;
	}
	rx = u.ux + u.dx; ry = u.uy + u.dy;
	if (!isok(rx,ry)) {
		You_hear("某人很小声的按屏幕.");
		return 0;
	}
	if ((mtmp = m_at(rx,ry)) != 0) {
		mstatusline(mtmp);
		if (mtmp->mundetected) {
			mtmp->mundetected = 0;
			if (cansee(rx,ry)) newsym(mtmp->mx,mtmp->my);
		}
		if (!canspotmon(mtmp))
			map_invisible(rx,ry);
		return res;
	}
	if (memory_is_invisible(rx, ry)) {
		unmap_object(rx, ry);
		newsym(rx, ry);
		pline_The("那个看不见的怪物肯定已经不在原地了.");
	}
	lev = &levl[rx][ry];
	switch(lev->typ) {
	case SDOOR:
		You_hear(hollow_str, "暗门");
		cvt_sdoor_to_door(lev);		/* ->typ = DOOR */
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	case SCORR:
		You_hear(hollow_str, "暗道");
		lev->typ = CORR;
		unblock_point(rx,ry);
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	}

	if (!its_dead(rx, ry, &res))
	    You("没听见什么不同的.");	/* not You_hear()  */
	return res;
}

static const char whistle_str[] = "吹出了%s的哨音.";

STATIC_OVL void
use_whistle(obj)
struct obj *obj;
{
	You(whistle_str, obj->cursed ? "尖锐的" : "很高的");
	wake_nearby();
}

STATIC_OVL void
use_magic_whistle(obj)
struct obj *obj;
{
	register struct monst *mtmp, *nextmon;

	if(obj->cursed && !rn2(2)) {
		You("搞出了一阵尖锐的噪音.");
		wake_nearby();
	} else {
		int pet_cnt = 0;
		You(whistle_str, Hallucination ? "普通的" : "怪异的");
		for(mtmp = fmon; mtmp; mtmp = nextmon) {
		    nextmon = mtmp->nmon; /* trap might kill mon */
		    if (DEADMONSTER(mtmp)) continue;
		    if (mtmp->mtame) {
			if (mtmp->mtrapped) {
			    /* no longer in previous trap (affects mintrap) */
			    mtmp->mtrapped = 0;
			    fill_pit(mtmp->mx, mtmp->my);
			}
			mnexto(mtmp);
			if (canspotmon(mtmp)) ++pet_cnt;
			if (mintrap(mtmp) == 2) change_luck(-1);
		    }
		}
		if (pet_cnt > 0) makeknown(obj->otyp);
	}
}

boolean
um_dist(x,y,n)
register xchar x, y, n;
{
	return((boolean)(abs(u.ux - x) > n  || abs(u.uy - y) > n));
}

int
number_leashed()
{
	register int i = 0;
	register struct obj *obj;

	for(obj = invent; obj; obj = obj->nobj)
		if(obj->otyp == LEASH && obj->leashmon != 0) i++;
	return(i);
}

void
o_unleash(otmp)		/* otmp is about to be destroyed or stolen */
register struct obj *otmp;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->m_id == (unsigned)otmp->leashmon)
			mtmp->mleashed = 0;
	otmp->leashmon = 0;
}

void
m_unleash(mtmp, feedback)	/* mtmp is about to die, or become untame */
register struct monst *mtmp;
boolean feedback;
{
	register struct obj *otmp;

	if (feedback) {
	    if (canseemon(mtmp))
		pline("%s从%s的项圈里头挣脱出来!", Monnam(mtmp), mhis(mtmp));
	    else
		Your("的项圈掉在地上.");
	}
	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH &&
				otmp->leashmon == (int)mtmp->m_id)
			otmp->leashmon = 0;
	mtmp->mleashed = 0;
}

void
unleash_all()		/* player is about to die (for bones) */
{
	register struct obj *otmp;
	register struct monst *mtmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH) otmp->leashmon = 0;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		mtmp->mleashed = 0;
}

#define MAXLEASHED	2

/* ARGSUSED */
STATIC_OVL void
use_leash(obj)
struct obj *obj;
{
	coord cc;
	register struct monst *mtmp;
	int spotmon;

	if(!obj->leashmon && number_leashed() >= MAXLEASHED) {
		You("没办法再栓别的怪物了.");
		return;
	}

	if(!get_adjacent_loc((char *)0, (char *)0, u.ux, u.uy, &cc)) return;

	if((cc.x == u.ux) && (cc.y == u.uy)) {
#ifdef STEED
		if (u.usteed && u.dz > 0) {
		    mtmp = u.usteed;
		    spotmon = 1;
		    goto got_target;
		}
#endif
		pline("给你自己戴项圈？别开玩笑了。");
		return;
	}

	if(!(mtmp = m_at(cc.x, cc.y))) {
		There("没有怪物能让你栓.");
		return;
	}

	spotmon = canspotmon(mtmp);
#ifdef STEED
 got_target:
#endif

	/* KMH, balance patch -- This doesn't work properly.
	 * Pets need extra memory for their edog structure.
	 * Normally, this is handled by tamedog(), but that
	 * rejects all demons.  Our other alternative would
	 * be to duplicate tamedog()'s functionality here.
	 * Yuck.  So I've merged it into the nymph code below.
	if (((mtmp->data == &mons[PM_SUCCUBUS]) || (mtmp->data == &mons[PM_INCUBUS]))
	     && (!mtmp->mtame) && (spotmon) && (!mtmp->mleashed)) {
	       pline("%s smiles seductively at the sight of this prop!", Monnam(mtmp));
	       mtmp->mtame = 10;
	       mtmp->mpeaceful = 1;
	       set_malign(mtmp);
	}*/
	if ((mtmp->data->mlet == S_NYMPH || mtmp->data == &mons[PM_SUCCUBUS]
		 || mtmp->data == &mons[PM_INCUBUS])
	     && (spotmon) && (!mtmp->mleashed)) {
	       pline("%s看起来被吓到了! \"我才不要和你玩sm!\"", Monnam(mtmp));
	       mtmp->mtame = 0;
	       mtmp->mpeaceful = 0;
	       mtmp->msleeping = 0;
	}
	if(!mtmp->mtame) {
	    if(!spotmon)
		There("没有怪物.");
	    else
		pline("%s %s拴住!", Monnam(mtmp), (!obj->leashmon) ?
				"没法被" : "不能被");
	    return;
	}
	if(!obj->leashmon) {
		if(mtmp->mleashed) {
			pline("那个%s已经被拴住了.",
			      spotmon ? l_monnam(mtmp) : "怪物");
			return;
		}
		You("收紧%s%s周围的拴绳.",
		    spotmon ? "你的 " : "", l_monnam(mtmp));
		mtmp->mleashed = 1;
		obj->leashmon = (int)mtmp->m_id;
		mtmp->msleeping = 0;
		return;
	}
	if(obj->leashmon != (int)mtmp->m_id) {
		pline("这个怪物身上没有拴着你的项圈.");
		return;
	} else {
		if(obj->cursed) {
			pline_The("你摘不掉那个项圈!");
			obj->bknown = TRUE;
			return;
		}
		mtmp->mleashed = 0;
		obj->leashmon = 0;
		You("从%s%s身上摘下项圈.",
		    spotmon ? "你的 " : "", l_monnam(mtmp));
		/* KMH, balance patch -- this is okay */
		if ((mtmp->data == &mons[PM_SUCCUBUS]) ||
				(mtmp->data == &mons[PM_INCUBUS]))
		{
		    pline("%s被激怒了!", Monnam(mtmp));
		    mtmp->mtame = 0;
		    mtmp->mpeaceful = 0;
		}

	}
	return;
}

struct obj *
get_mleash(mtmp)	/* assuming mtmp->mleashed has been checked */
register struct monst *mtmp;
{
	register struct obj *otmp;

	otmp = invent;
	while(otmp) {
		if(otmp->otyp == LEASH && otmp->leashmon == (int)mtmp->m_id)
			return(otmp);
		otmp = otmp->nobj;
	}
	return((struct obj *)0);
}

#endif /* OVLB */
#ifdef OVL1

boolean
next_to_u()
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if(mtmp->mleashed) {
			if (distu(mtmp->mx,mtmp->my) > 2) mnexto(mtmp);
			if (distu(mtmp->mx,mtmp->my) > 2) {
			    for(otmp = invent; otmp; otmp = otmp->nobj)
				if(otmp->otyp == LEASH &&
					otmp->leashmon == (int)mtmp->m_id) {
				    if(otmp->cursed) return(FALSE);
				    You_feel("%s项圈松掉了.",
					(number_leashed() > 1) ? "一个" : "那个");
				    mtmp->mleashed = 0;
				    otmp->leashmon = 0;
				}
			}
		}
	}
#ifdef STEED
	/* no pack mules for the Amulet */
	if (u.usteed && mon_has_amulet(u.usteed)) return FALSE;
#endif
	return(TRUE);
}

#endif /* OVL1 */
#ifdef OVL0

void
check_leash(x, y)
register xchar x, y;
{
	register struct obj *otmp;
	register struct monst *mtmp;

	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (otmp->otyp != LEASH || otmp->leashmon == 0) continue;
	    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if ((int)mtmp->m_id == otmp->leashmon) break; 
	    }
	    if (!mtmp) {
		impossible("错误！正在使用的项圈没有拴住一个存在的怪物?");
		otmp->leashmon = 0;
		continue;
	    }
	    if (dist2(u.ux,u.uy,mtmp->mx,mtmp->my) >
		    dist2(x,y,mtmp->mx,mtmp->my)) {
		if (!um_dist(mtmp->mx, mtmp->my, 3)) {
		    ;	/* still close enough */
		} else if (otmp->cursed && !breathless(mtmp->data)) {
		    if (um_dist(mtmp->mx, mtmp->my, 5) ||
			    (mtmp->mhp -= rnd(2)) <= 0) {
			long save_pacifism = u.uconduct.killer;

			Your("的项圈把%s勒到窒息而死!", mon_nam(mtmp));
			/* hero might not have intended to kill pet, but
			   that's the result of his actions; gain experience,
			   lose pacifism, take alignment and luck hit, make
			   corpse less likely to remain tame after revival */
			xkilled(mtmp, 0);	/* no "you kill it" message */
			/* life-saving doesn't ordinarily reset this */
			if (mtmp->mhp > 0) u.uconduct.killer = save_pacifism;
		    } else {
			pline("%s被项圈勒死了!", Monnam(mtmp));
			/* tameness eventually drops to 1 here (never 0) */
			if (mtmp->mtame && rn2(mtmp->mtame)) mtmp->mtame--;
		    }
		} else {
		    if (um_dist(mtmp->mx, mtmp->my, 5)) {
			pline("%s的项圈猛地崩开了!", s_suffix(Monnam(mtmp)));
			m_unleash(mtmp, FALSE);
		    } else {
			You("拉拴绳.");
			if (mtmp->data->msound != MS_SILENT)
			    switch (rn2(3)) {
			    case 0:  growl(mtmp);   break;
			    case 1:  yelp(mtmp);    break;
			    default: whimper(mtmp); break;
			    }
		    }
		}
	    }
	}
}

#endif /* OVL0 */
#ifdef OVLB

#define WEAK	3	/* from eat.c */

static const char look_str[] = "look %s.";

STATIC_OVL int
use_mirror(obj)
struct obj *obj;
{
	register struct monst *mtmp;
	register char mlet;
#ifdef INVISIBLE_OBJECTS
	boolean vis = !Blind && (!obj->oinvis || See_invisible);
#else
	boolean vis = !Blind;
#endif

	if(!getdir((char *)0)) return 0;
	if(obj->cursed && !rn2(2)) {
		if (vis)
			pline_The("镜子起雾了，导致上面根本没有反光的效果!");
		return 1;
	}
	if(!u.dx && !u.dy && !u.dz) {
		if(vis && !Invisible) {
		    if (u.umonnum == PM_FLOATING_EYE) {
			if (!Free_action) {
			pline(Hallucination ?
			      "妈呀，镜子瞪你!" :
			      "哎呦！你把你自己吓到了!");
			nomul(-rnd((MAXULEV+6) - u.ulevel));
			nomovemsg = 0;
			} else You("在你的瞪视之中颤抖了一下.");
		    } else if (is_vampire(youmonst.data))
			You("没有倒影.");
		    else if (u.umonnum == PM_UMBER_HULK) {
			pline("啥玩意，这是你吗？");
			make_confused(HConfusion + d(3,4),FALSE);
		    } else if (Hallucination)
			You(look_str, hcolor((char *)0));
		    else if (Sick)
			You(look_str, "形容枯槁");
		    else if (u.uhs >= WEAK)
			You(look_str, "饿的不成人样");
		    else You("看起来像往常一样%s.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美丽的" : "帅气的") :
				"丑不拉几的");
		} else {
			You_cant("没法看见你的%s %s.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美丽的" : "帅气的") :
				"丑不拉几的",
				body_part(FACE));
		}
		return 1;
	}
	if(u.uswallow) {
		if (vis) You("的镜子照出%s的%s.", s_suffix(mon_nam(u.ustuck)),
		    mbodypart(u.ustuck, STOMACH));
		return 1;
	}
	if(Underwater) {
#ifdef INVISIBLE_OBJECTS
		if (!obj->oinvis)
#endif
		You(Hallucination ?
		    "给水里的鱼儿们了一个让他们修整仪表的机会." :
		    "的镜子里头全是脏兮兮的水.");
		return 1;
	}
	if(u.dz) {
		if (vis)
		    You("的镜子里照出%s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		return 1;
	}
	mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
		    (int FDECL((*),(MONST_P,OBJ_P)))0,
		    (int FDECL((*),(OBJ_P,OBJ_P)))0,
		    &obj);
	if (!mtmp || !haseyes(mtmp->data))
		return 1;

	vis = canseemon(mtmp);
	mlet = mtmp->data->mlet;
	if (mtmp->msleeping) {
		if (vis)
		    pline ("%s累的无暇顾及你的镜子.",
			    Monnam(mtmp));
	} else if (!mtmp->mcansee) {
	    if (vis)
		pline("%s现在啥也看不见，更别提你的镜子了.", Monnam(mtmp));
#ifdef INVISIBLE_OBJECTS
	} else if (obj->oinvis && !perceives(mtmp->data)) {
	    if (vis)
		pline("%s看不到你手里的镜子.", Monnam(mtmp));
#endif
	/* some monsters do special things */
	} else if (is_vampire(mtmp->data) || mlet == S_GHOST) {
	    if (vis)
		pline ("%s压根就没有倒影.", Monnam(mtmp));
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_MEDUSA]) {
		if (mon_reflects(mtmp, "美杜莎的眼神被手中的%s的%s!"))
			return 1;
		if (vis)
			pline("%s变成了石头!", Monnam(mtmp));
		stoned = TRUE;
		killed(mtmp);
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_FLOATING_EYE]) {
		int tmp = d((int)mtmp->m_lev, (int)mtmp->data->mattk[0].damd);
		if (!rn2(4)) tmp = 120;
		if (vis)
			pline("%s被它自己的目光搞得麻痹了.", Monnam(mtmp));
		else You_hear("%s停止了移动.",something);
		mtmp->mcanmove = 0;
		if ( (int) mtmp->mfrozen + tmp > 127)
			mtmp->mfrozen = 127;
		else mtmp->mfrozen += tmp;
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_UMBER_HULK]) {
		if (vis)
			pline ("%s把它自己搞迷糊了!", Monnam(mtmp));
		mtmp->mconf = 1;
	} else if(!mtmp->mcan && !mtmp->minvis && (mlet == S_NYMPH
				     || mtmp->data==&mons[PM_SUCCUBUS])) {
		if (vis) {
		    pline ("%s对着你的镜子理了理头发笑了笑，它很欣赏里头的自己.", Monnam(mtmp));
		    pline ("她把镜子拿走了!");
		} else pline ("它把你的镜子偷走了!");
		setnotworn(obj); /* in case mirror was wielded */
		freeinv(obj);
		(void) mpickobj(mtmp,obj);
		if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
	} else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data) &&
			(!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
		if (vis)
		    pline("%s被它在镜子里头的倒影吓到了.", Monnam(mtmp));
		monflee(mtmp, d(2,4), FALSE, FALSE);
	} else if (!Blind) {
		if (mtmp->minvis && !See_invisible)
		    ;
		else if ((mtmp->minvis && !perceives(mtmp->data))
			 || !haseyes(mtmp->data))
		    pline("%s根本没注意到它的倒影.",
			Monnam(mtmp));
		else
		    pline("%s无视了%s的倒影.",
			  Monnam(mtmp), mhis(mtmp));
	}
	return 1;
}

STATIC_OVL void
use_bell(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	struct monst *mtmp;
	boolean wakem = FALSE, learno = FALSE,
		ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
		invoking = (obj->otyp == BELL_OF_OPENING &&
			 invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy));

	You("敲响了%s.", the(xname(obj)));

	if (Underwater || (u.uswallow && ordinary)) {
#ifdef	AMIGA
	    amii_speaker( obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME );
#endif
	    pline("但是没啥声音.");

	} else if (invoking && ordinary) {
	    /* needs to be recharged... */
	    pline("但是它没发出声音.");
	    learno = TRUE;	/* help player figure out why */

	} else if (ordinary) {
#ifdef	AMIGA
	    amii_speaker( obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME );
#endif
	    if (obj->cursed && !rn2(4) &&
		    /* note: once any of them are gone, we stop all of them */
		    !(mvitals[PM_WOOD_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE) &&
		    (mtmp = makemon(mkclass(S_NYMPH, 0),
					u.ux, u.uy, NO_MINVENT)) != 0) {
		You("召唤了一个%s!", a_monnam(mtmp));
		if (!obj_resists(obj, 93, 100)) {
		    pline("%s破碎了!", Tobjnam(obj, "突然"));
		    useup(obj);
		    *optr = 0;
		} else switch (rn2(3)) {
			default:
				break;
			case 1:
				mon_adjust_speed(mtmp, 2, (struct obj *)0);
				break;
			case 2: /* no explanation; it just happens... */
				nomovemsg = "";
				nomul(-rnd(2));
				break;
		}
	    }
	    wakem = TRUE;

	} else {
	    /* charged Bell of Opening */
	    consume_obj_charge(obj, TRUE);

	    if (u.uswallow) {
		if (!obj->cursed)
		    (void) openit();
		else
		    pline(nothing_happens);

	    } else if (obj->cursed) {
		coord mm;

		mm.x = u.ux;
		mm.y = u.uy;
		mkundead(&mm, FALSE, NO_MINVENT);
		wakem = TRUE;

	    } else  if (invoking) {
		pline("%s制造出一阵让人毛骨悚然的声音...",
		      Tobjnam(obj, "issue"));
#ifdef	AMIGA
		amii_speaker( obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME );
#endif
		obj->age = moves;
		learno = TRUE;
		wakem = TRUE;

	    } else if (obj->blessed) {
		int res = 0;

#ifdef	AMIGA
		amii_speaker( obj, "ahahahDhEhCw", AMII_SOFT_VOLUME );
#endif
		if (uchain) {
		    unpunish();
		    res = 1;
		}
		res += openit();
		switch (res) {
		  case 0:  pline(nothing_happens); break;
		  case 1:  pline("%s打开了...", Something);
			   learno = TRUE; break;
		  default: pline("在你周围的物体打开了...");
			   learno = TRUE; break;
		}

	    } else {  /* uncursed */
#ifdef	AMIGA
		amii_speaker( obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME );
#endif
		if (findit() != 0) learno = TRUE;
		else pline(nothing_happens);
	    }

	}	/* charged BofO */

	if (learno) {
	    makeknown(BELL_OF_OPENING);
	    obj->known = 1;
	}
	if (wakem) wake_nearby();
}

STATIC_OVL void
use_candelabrum(obj)
register struct obj *obj;
{
	const char *s = (obj->spe != 1) ? "蜡烛" : "蜡烛";

	if(Underwater) {
		You("在水底下没法点火。");
		return;
	}
	if(obj->lamplit) {
		You("吹灭了%s.", s);
		end_burn(obj, TRUE);
		return;
	}
	if(obj->spe <= 0) {
		pline("这个%s已经没有%s了.", xname(obj), s);
		return;
	}
	if(u.uswallow || obj->cursed) {
		if (!Blind)
		    pline_The("%s %s了一会,然后%s了.",
			      s, vtense(s, "闪烁"), vtense(s, "熄灭"));
		return;
	}
	if(obj->spe < 7) {
		There("%s只有%d %s in %s.",
		      vtense(s, "are"), obj->spe, s, the(xname(obj)));
		if (!Blind)
		    pline("%s被点燃了.你能感受到微弱的%s.",
			  obj->spe == 1 ? "它" : "它们",
			  Tobjnam(obj, "亮光"));
	} else {
		pline("%s在%s燃烧中放出明亮的%s", The(xname(obj)), s,
			(Blind ? "." : " 光芒!"));
	}
	if (!invocation_pos(u.ux, u.uy)) {
		pline_The("%s%s被快速的消耗着！", s, vtense(s, "在"));
		obj->age /= 2;
	} else {
		if(obj->spe == 7) {
		    if (Blind)
		      pline("%s出一股奇怪的的热!", Tobjnam(obj, "放射"));
		    else
		      pline("%s一阵奇怪的光亮!", Tobjnam(obj, "冒出"));
		}
		obj->known = 1;
	}
	begin_burn(obj, FALSE);
}

STATIC_OVL void
use_candle(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	register struct obj *otmp;
	const char *s = (obj->quan != 1) ? "蜡烛" : "蜡烛";
	char qbuf[QBUFSZ];

	if(u.uswallow) {
		You(no_elbow_room);
		return;
	}
	if(Underwater) {
		pline("抱歉，但是水和火不能相融.");
		return;
	}

	otmp = carrying(CANDELABRUM_OF_INVOCATION);
	/* [ALI] Artifact candles can't be attached to candelabrum
	 *       (magic candles still can be).
	 */
	if(obj->oartifact || !otmp || otmp->spe == 7) {
		use_lamp(obj);
		return;
	}

	Sprintf(qbuf, " 把%s", the(xname(obj)));
	Sprintf(eos(qbuf), "添加到%s?",
		safe_qbuf(qbuf, sizeof(" to ?"), the(xname(otmp)),
			the(simple_typename(otmp->otyp)), "it"));
	if(yn(qbuf) == 'n') {
		if (!obj->lamplit)
		    You("试图点燃%s...", the(xname(obj)));
		use_lamp(obj);
		return;
	} else {
		if ((long)otmp->spe + obj->quan > 7L)
		    obj = splitobj(obj, 7L - (long)otmp->spe);
		else *optr = 0;
		You("把更多的%ld%s %s放到%s上.",
		    obj->quan, !otmp->spe ? "" : " more",
		    s, the(xname(otmp)));
		if (obj->otyp == MAGIC_CANDLE) {
		    if (obj->lamplit)
			pline_The("这个%s%s很新.", s,
				vtense(s, "看起来"));
		    else
			pline("%s看起来没什么不同的.",
				(obj->quan > 1L) ? "它们" : "它");
		    if (!otmp->spe)
			otmp->age = 600L;
		} else
		if (!otmp->spe || otmp->age > obj->age)
		    otmp->age = obj->age;
		otmp->spe += (int)obj->quan;
		if (otmp->lamplit && !obj->lamplit)
		    pline_The("new %s magically %s!", s, vtense(s, "点燃了"));
		else if (!otmp->lamplit && obj->lamplit)
		    pline("%s烧完了.", (obj->quan > 1L) ? "它们" : "它");
		if (obj->unpaid)
		    verbalize("你%s%s,所以你必须买下%s!",
			      otmp->lamplit ? "点着了" : "使用了",
			      (obj->quan > 1L) ? "它" : "它",
			      (obj->quan > 1L) ? "它" : "它");
		if (obj->quan < 7L && otmp->spe == 7)
		    pline("%s现在上面已经被放入了七根蜡烛了.",
			  The(xname(otmp)), otmp->lamplit ? " 被点燃的" : "");
		/* candelabrum's light range might increase */
		if (otmp->lamplit) obj_merge_light_sources(otmp, otmp);
		/* candles are no longer a separate light source */
		if (obj->lamplit) end_burn(obj, TRUE);
		/* candles are now gone */
		useupall(obj);
	}
}

boolean
snuff_candle(otmp)  /* call in drop, throw, and put in box, etc. */
register struct obj *otmp;
{
	register boolean candle = Is_candle(otmp);

	if (((candle && otmp->oartifact != ART_CANDLE_OF_ETERNAL_FLAME)
		|| otmp->otyp == CANDELABRUM_OF_INVOCATION) &&
		otmp->lamplit) {
	    char buf[BUFSZ];
	    xchar x, y;
	    register boolean many = candle ? otmp->quan > 1L : otmp->spe > 1;

	    (void) get_obj_location(otmp, &x, &y, 0);
	    if (otmp->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
		pline("%s上面%s蜡烛%s的火焰%s熄灭了.",
		      Shk_Your(buf, otmp),
		      (candle ? "" : "祷告烛台 "),
		      (many ? "的" : "的"), (many ? "已经" : "已经"));
	   end_burn(otmp, TRUE);
	   return(TRUE);
	}
	return(FALSE);
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
	xchar x, y;

	if (obj->lamplit) {
	    if (artifact_light(obj)) return FALSE; /* Artifact lights are never snuffed */
	    if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
		obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL ||
		obj->otyp == TORCH) {
		(void) get_obj_location(obj, &x, &y, 0);
		if (obj->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
		    pline("%s %s out!", Yname2(obj), otense(obj, "go"));
		end_burn(obj, TRUE);
		return TRUE;
	    }
	    if (snuff_candle(obj)) return TRUE;
	}
	return FALSE;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object was lit and FALSE otherwise --ALI */
boolean
catch_lit(obj)
struct obj *obj;
{
	xchar x, y;

	if (!obj->lamplit && (obj->otyp == MAGIC_LAMP || ignitable(obj))) {
	    if ((obj->otyp == MAGIC_LAMP ||
		 obj->otyp == CANDELABRUM_OF_INVOCATION) &&
		obj->spe == 0)
		return FALSE;
	    else if (obj->otyp != MAGIC_LAMP && obj->age == 0)
		return FALSE;
	    if (!get_obj_location(obj, &x, &y, 0))
		return FALSE;
	    if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
		return FALSE;
	    if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
		 obj->otyp == BRASS_LANTERN) && obj->cursed && !rn2(2))
		return FALSE;
	    if (obj->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
		pline("%s %s light!", Yname2(obj), otense(obj, "catch"));
	    if (obj->otyp == POT_OIL) makeknown(obj->otyp);
	    if (obj->unpaid && costly_spot(u.ux, u.uy) && (obj->where == OBJ_INVENT)) {
	        /* if it catches while you have it, then it's your tough luck */
		check_unpaid(obj);
	        verbalize("That's in addition to the cost of %s %s, of course.",
				Yname2(obj), obj->quan == 1 ? "itself" : "themselves");
		bill_dummy_object(obj);
	    }
	    begin_burn(obj, FALSE);
	    return TRUE;
	}
	return FALSE;
}

STATIC_OVL void
use_lamp(obj)
struct obj *obj;
{
	char buf[BUFSZ];
	char qbuf[QBUFSZ];

	if(Underwater) {
		pline("这不是潜水灯.");
		return;
	}
	if(obj->lamplit) {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN) {
		    pline("%s现在灭掉了.", Shk_Your(buf, obj));
#ifdef LIGHTSABERS
		} else if(is_lightsaber(obj)) {
		    if (obj->otyp == RED_DOUBLE_LIGHTSABER) {
			/* Do we want to activate dual bladed mode? */
			if (!obj->altmode && (!obj->cursed || rn2(4))) {
			    You("打开了%s的照明模式.", yname(obj));
			    obj->altmode = TRUE;
			    return;
			} else obj->altmode = FALSE;
		    }
		    lightsaber_deactivate(obj, TRUE);
		    return;
#endif
		} else if (artifact_light(obj)) {
		    You_cant("把%s灭掉.", yname(obj));
		    return;
		} else {
		    You("把%s关掉了.", yname(obj));
		}
		end_burn(obj, TRUE);
		return;
	}
	/* magic lamps with an spe == 0 (wished for) cannot be lit */
	if ((!Is_candle(obj) && obj->age == 0)
			|| (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
		if ((obj->otyp == BRASS_LANTERN)
#ifdef LIGHTSABERS
			|| is_lightsaber(obj)
#endif
			)
			Your("%s没电了.", xname(obj));
		else if (obj->otyp == TORCH) {
		        Your("的火把上的可燃物烧干净了，没法再被点燃了.");
		}
		else pline("%s没油了.", xname(obj));
		return;
	}
	if (obj->cursed && !rn2(2)) {
		pline("火苗%s了一会,然后%s.",
		      Tobjnam(obj, "闪烁"), otense(obj, "熄灭了"));
	} else {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN) {
		    check_unpaid(obj);
		    pline("%s灯现在放射出光亮.", Shk_Your(buf, obj));
		} else if (obj->otyp == TORCH) {
		    check_unpaid(obj);
		    pline("%s的火苗%s燃烧的%s%s",
			s_suffix(Yname2(obj)),
			plur(obj->quan),
			obj->quan > 1L ? "" : "s",
			Blind ? "." : " 非常亮!");
#ifdef LIGHTSABERS
		} else if (is_lightsaber(obj)) {
		    /* WAC -- lightsabers */
		    /* you can see the color of the blade */
		    
		    if (!Blind) makeknown(obj->otyp);
		    You("按下了%s的照明开关.", yname(obj));
		    unweapon = FALSE;
#endif
		} else {	/* candle(s) */
		    Sprintf(qbuf, "把全部的%s都点燃?", the(xname(obj)));
		    if (obj->quan > 1L && (yn(qbuf) == 'n')) {
			/* Check if player wants to light all the candles */
			struct obj *rest;	     /* the remaining candles */
			rest = splitobj(obj, obj->quan - 1L);
			obj_extract_self(rest);	     /* free from inv */
			obj->spe++;	/* this prevents merging */
			(void)hold_another_object(rest, "你把%s丢掉了!",
					  doname(rest), (const char *)0);
			obj->spe--;
		    }
		    pline("%s的火焰%s %s%s",
			s_suffix(Yname2(obj)),
			plur(obj->quan), otense(obj, "燃烧的"),
			Blind ? "." : "非常亮!");
		    if (obj->unpaid && costly_spot(u.ux, u.uy) &&
			  obj->otyp != MAGIC_CANDLE) {
			const char *ithem = obj->quan > 1L ? "它们" : "它";
			verbalize("你把%s点着了,你就必须要买下%s!", ithem, ithem);
			bill_dummy_object(obj);
		    }
		}
		begin_burn(obj, FALSE);
	}
}

/* MRKR: Torches */

STATIC_OVL int
use_torch(obj)
struct obj *obj;
{
    struct obj *otmp = NULL;
    if (u.uswallow) {
	You(no_elbow_room);
	return 0;
    }
    if (Underwater) {
	pline("抱歉，水和火没法相容.");
	return 0;
    }
    if (obj->quan > 1L) {
	otmp = obj;
	obj = splitobj(otmp, 1L);
	obj_extract_self(otmp);	/* free from inv */
    }
    /* You can use a torch in either wielded weapon slot */
    if (obj != uwep && (obj != uswapwep || !u.twoweap))
	if (!wield_tool(obj, (const char *)0)) return 0;
    use_lamp(obj);
    /* shouldn't merge */
    if (otmp)
	otmp = hold_another_object(otmp, "你把%s丢掉了!",
				   doname(otmp), (const char *)0);
    return 1;
}

STATIC_OVL void
light_cocktail(obj)
	struct obj *obj;        /* obj is a potion of oil or a stick of dynamite */
{
	char buf[BUFSZ];
	const char *objnam =
#ifdef FIREARMS
	    obj->otyp == POT_OIL ? "药水" : "魔杖";
#else
	    "potion";
#endif

	if (u.uswallow) {
	    You(no_elbow_room);
	    return;
	}

	if(Underwater) {
		You("没法把它在水底下点着!");
		return;
	}

	if (obj->lamplit) {
	    You("把%s的火苗熄灭了.", objnam);
	    end_burn(obj, TRUE);
	    /*
	     * Free & add to re-merge potion.  This will average the
	     * age of the potions.  Not exactly the best solution,
	     * but its easy.
	     */
	    freeinv(obj);
	    (void) addinv(obj);
	    return;
	} else if (Underwater) {
	    There("在这里可没有足够的氧气来让火苗燃烧.");
	    return;
	}

	You("点着了%s %s.%s", shk_your(buf, obj), objnam,
	    Blind ? "" : " 它放出很微弱的光.");
	if (obj->unpaid && costly_spot(u.ux, u.uy)) {
	    /* Normally, we shouldn't both partially and fully charge
	     * for an item, but (Yendorian Fuel) Taxes are inevitable...
	     */
#ifdef FIREARMS
	    if (obj->otyp != STICK_OF_DYNAMITE) {
#endif
	    check_unpaid(obj);
	    verbalize("哦，对这瓶药水的损耗我们要额外收钱.");
#ifdef FIREARMS
	    } else {
		const char *ithem = obj->quan > 1L ? "它们" : "它";
		verbalize("你把%s点着了,你就得把%s买下来!", ithem, ithem);
	    }
#endif
	    bill_dummy_object(obj);
	}
	makeknown(obj->otyp);
#ifdef FIREARMS
	if (obj->otyp == STICK_OF_DYNAMITE) obj->yours=TRUE;
#endif

	if (obj->quan > 1L) {
	    obj = splitobj(obj, 1L);
	    begin_burn(obj, FALSE);	/* burn before free to get position */
	    obj_extract_self(obj);	/* free from inv */

	    /* shouldn't merge */
	    obj = hold_another_object(obj, "你把%s丢掉了!",
				      doname(obj), (const char *)0);
	} else
	    begin_burn(obj, FALSE);
}

static NEARDATA const char cuddly[] = { TOOL_CLASS, GEM_CLASS, 0 };

int
dorub()
{
	struct obj *obj = getobj(cuddly, "rub");

	if (obj && obj->oclass == GEM_CLASS) {
	    if (is_graystone(obj)) {
		use_stone(obj);
		return 1;
	    } else {
		pline("你没法这么干，试试别的吧.");
		return 0;
	    }
	}

	if (!obj || !wield_tool(obj, "rub")) return 0;

	/* now uwep is obj */
	if (uwep->otyp == MAGIC_LAMP) {
	    if (uwep->spe > 0 && !rn2(3)) {
		check_unpaid_usage(uwep, TRUE);		/* unusual item use */
		djinni_from_bottle(uwep);
		makeknown(MAGIC_LAMP);
		uwep->otyp = OIL_LAMP;
		uwep->spe = 0; /* for safety */
		uwep->age = rn1(500,1000);
		if (uwep->lamplit) begin_burn(uwep, TRUE);
		update_inventory();
	    } else if (rn2(2) && !Blind)
		You("看见一阵烟雾冒出来.");
	    else pline(nothing_happens);
	} else if (obj->otyp == BRASS_LANTERN) {
	    /* message from Adventure */
	    pline("摩擦一个现代化的灯什么事也不会发生.");
	    pline("没啥有趣的事发生.");
	} else pline(nothing_happens);
	return 1;
}

int
dojump()
{
	/* Physical jump */
	return jump(0);
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
	coord cc;

	if (!magic && (nolimbs(youmonst.data) || slithy(youmonst.data))) {
		/* normally (nolimbs || slithy) implies !Jumping,
		   but that isn't necessarily the case for knights */
		You_cant("跳跃，你根本就没有腿!");
		return 0;
	} else if (!magic && !Jumping) {
		You_cant("跳的很远.");
		return 0;
	} else if (u.uswallow) {
		if (magic) {
			You("绕着周围跳了一圈.");
			return 1;
		} else {
		pline("你开玩笑的吧!");
		return 0;
		}
		return 0;
	} else if (u.uinwater) {
		if (magic) {
			You("绕着周围游了一圈.");
			return 1;
		} else {
		pline("这叫游泳，不叫跳跃!白痴!");
		return 0;
		}
		return 0;
	} else if (u.ustuck) {
		if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
		    You("从%s挣脱出来.", mon_nam(u.ustuck));
		    setustuck(0);
		    return 1;
		}
		if (magic) {
			You("从%s的束缚中稍微地挣脱出来了一点!", mon_nam(u.ustuck));
			return 1;
		} else {
		You("没法从%s挣脱出来!", mon_nam(u.ustuck));
		return 0;
		}

		return 0;
	} else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		if (magic) {
			You("在周围漂浮着飞了一圈.");
			return 1;
		} else {
		You("don't have enough traction to jump.");
		return 0;
		}
	} else if (!magic && near_capacity() > UNENCUMBERED) {
		You("身上的东西太多没法跳起来！");
		return 0;
	} else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
		You("缺少跳起来的力气!");
		return 0;
	} else if (Wounded_legs) {
 		long wl = (EWounded_legs & BOTH_SIDES);
		const char *bp = body_part(LEG);

		if (wl == BOTH_SIDES) bp = makeplural(bp);
#ifdef STEED
		if (u.usteed)
		    pline("%s的状况很不适合进行跳跃.", Monnam(u.usteed));
		else
#endif
		Your("%s%s %s不适合进行跳跃.",
		     (wl == LEFT_SIDE) ? "左" :
			(wl == RIGHT_SIDE) ? "右" : "",
		     bp, (wl == BOTH_SIDES) ? "都" : "全部都");
		return 0;
	}
#ifdef STEED
	else if (u.usteed && u.utrap) {
		pline("%s被卡在捕兽夹里头动弹不得.", Monnam(u.usteed));
		return (0);
	}
#endif

	pline("你想跳到哪里?");
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, "the desired position") < 0)
		return 0;	/* user pressed ESC */
	if (!magic && !(HJumping & ~INTRINSIC) && !EJumping &&
			distu(cc.x, cc.y) != 5) {
		/* The Knight jumping restriction still applies when riding a
		 * horse.  After all, what shape is the knight piece in chess?
		 */
		pline("你选的位置太离谱了!");
		return 0;
	} else if (distu(cc.x, cc.y) > (magic ? 6+magic*3 : 9)) {
		pline("太远了!");
		return 0;
	} else if (!cansee(cc.x, cc.y)) {
		You("看不见跳跃的目的地!");
		return 0;
	} else if (!isok(cc.x, cc.y)) {
		You("没法在这里跳跃!");
		return 0;
	} else {
	    coord uc;
	    int range, temp;

	    if(u.utrap)
		switch(u.utraptype) {
		case TT_BEARTRAP: {
		    register long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;
		    You("从捕兽夹里头强行把自己的腿拽了出来!疼死了!");
#ifdef STEED
			if (!u.usteed)
#endif
		    losehp(rnd(10), "试图从捕兽夹子里头蹦出来", KILLED_BY);
		    set_wounded_legs(side, rn1(1000,500));
		    break;
		  }
		case TT_PIT:
		    You("从坑里头跳了出来!");
		    break;
		case TT_WEB:
		    You("把周围的蜘蛛网直接撕碎了!");
		    deltrap(t_at(u.ux,u.uy));
		    break;
		case TT_LAVA:
		    You("把你自己从岩浆里头拉了出来!");
		    u.utrap = 0;
		    return 1;
		case TT_INFLOOR:
		    You("用尽全力试图把%s拉出来,但是你的腿还是卡在地板里头.",
			makeplural(body_part(LEG)));
		    set_wounded_legs(LEFT_SIDE, rn1(10, 11));
		    set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
		    return 1;
		}

	    /*
	     * Check the path from uc to cc, calling hurtle_step at each
	     * location.  The final position actually reached will be
	     * in cc.
	     */
	    uc.x = u.ux;
	    uc.y = u.uy;
	    /* calculate max(abs(dx), abs(dy)) as the range */
	    range = cc.x - uc.x;
	    if (range < 0) range = -range;
	    temp = cc.y - uc.y;
	    if (temp < 0) temp = -temp;
	    if (range < temp)
		range = temp;
	    (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t)&range);

	    /* A little Sokoban guilt... */
	    if (In_sokoban(&u.uz))
		change_luck(-1);

	    teleds(cc.x, cc.y, TRUE);
	    nomul(-1);
	    nomovemsg = "";
	    morehungry(rnd(25));
	    return 1;
	}
}

boolean
tinnable(corpse)
struct obj *corpse;
{
	if (corpse->otyp != CORPSE) return 0;
	if (corpse->oeaten) return 0;
	if (corpse->odrained) return 0;
	if (!mons[corpse->corpsenm].cnutrit) return 0;
	return 1;
}

STATIC_OVL void
use_tinning_kit(obj)
register struct obj *obj;
{
	register struct obj *corpse, *can;
/*
	char *badmove;
 */
	/* This takes only 1 move.  If this is to be changed to take many
	 * moves, we've got to deal with decaying corpses...
	 */
	if (obj->spe <= 0) {
		You("的装罐器里头空罐子用完了.");
		return;
	}
	if (!(corpse = getobj((const char *)tinnables, "tin"))) return;
	if (corpse->otyp == CORPSE && (corpse->oeaten || corpse->odrained)) {
		You("没法把已经吃了一半的%s做成罐头.",something);
		return;
	}
	if (!tinnable(corpse)) {
		You_cant("把那个尸体装罐!");
		return;
	}
	if (touch_petrifies(&mons[corpse->corpsenm])
		&& !Stone_resistance && !uarmg) {
	    char kbuf[BUFSZ];

	    if (poly_when_stoned(youmonst.data))
		You("在试图把%s做成罐头的时候  没 戴 手 套.",
			an(mons[corpse->corpsenm].mname));
	    else {
		pline("没带手套的时候把%s做成罐头真是个蠢的要死的错误...",
			an(mons[corpse->corpsenm].mname));
		Sprintf(kbuf, "试图不带手套把%s做成罐头",
			an(mons[corpse->corpsenm].mname));
	    }
	    instapetrify(kbuf);
	}
	if (is_rider(&mons[corpse->corpsenm])) {
		(void) revive_corpse(corpse, FALSE);
		verbalize("想法不错....但是战争从来不会把他的敌人保留着......");
		return;
	}
	if (mons[corpse->corpsenm].cnutrit == 0) {
		pline("这玩意装起来估计很快就要烂掉，没法装罐.");
		return;
	}
	consume_obj_charge(obj, TRUE);

	if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
	    static const char you_buy_it[] = "你把它做成了罐头，现在你必须把它买下！";

	    can->corpsenm = corpse->corpsenm;
	    can->cursed = obj->cursed;
	    can->blessed = obj->blessed;
	    can->owt = weight(can);
	    can->known = 1;
#ifdef EATEN_MEMORY
	    /* WAC You know the type of tinned corpses */
	    if (mvitals[corpse->corpsenm].eaten < 255) 
	    	mvitals[corpse->corpsenm].eaten++;
#endif    
	    can->spe = -1;  /* Mark tinned tins. No spinach allowed... */
	    if (carried(corpse)) {
		if (corpse->unpaid)
		    verbalize(you_buy_it);
		useup(corpse);
	    } else if (mcarried(corpse)) {
		m_useup(corpse->ocarry, corpse);
	    } else {
		if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
		    verbalize(you_buy_it);
		useupf(corpse, 1L);
	    }
	    can = hold_another_object(can, "你做成了罐头，但是没法把%s捡起来.",
				      doname(can), (const char *)0);
	} else impossible("装罐失败喽，你干了些什么？.");
}


void
use_unicorn_horn(obj)
struct obj *obj;
{
#define PROP_COUNT 6		/* number of properties we're dealing with */
#define ATTR_COUNT (A_MAX*3)	/* number of attribute points we might fix */
	int idx, val, val_limit,
	    trouble_count, unfixable_trbl, did_prop, did_attr;
	int trouble_list[PROP_COUNT + ATTR_COUNT];
	int chance;	/* KMH */

	if (obj && obj->cursed) {
	    long lcount = (long) rnd(100);

	    switch (rn2(6)) {
	    case 0: make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON),20),
			xname(obj), TRUE, SICK_NONVOMITABLE);
		    break;
	    case 1: make_blinded(Blinded + lcount, TRUE);
		    break;
	    case 2: if (!Confusion)
			You("突然感觉%s.",
			    Hallucination ? "被卡住了" : "很迷惑");
		    make_confused(HConfusion + lcount, TRUE);
		    break;
	    case 3: make_stunned(HStun + lcount, TRUE);
		    break;
	    case 4: (void) adjattrib(rn2(A_MAX), -1, FALSE);
		    break;
	    case 5: (void) make_hallucinated(HHallucination + lcount, TRUE, 0L);
		    break;
	    }
	    return;
	}

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop2trbl(X)	((X) + A_MAX)
#define attr2trbl(Y)	(Y)
#define prop_trouble(X) trouble_list[trouble_count++] = prop2trbl(X)
#define attr_trouble(Y) trouble_list[trouble_count++] = attr2trbl(Y)

	trouble_count = unfixable_trbl = did_prop = did_attr = 0;

	/* collect property troubles */
	if (Sick) prop_trouble(SICK);
	if (Blinded > (long)u.ucreamed) prop_trouble(BLINDED);
	if (HHallucination) prop_trouble(HALLUC);
	if (Vomiting) prop_trouble(VOMITING);
	if (HConfusion) prop_trouble(CONFUSION);
	if (HStun) prop_trouble(STUNNED);

	unfixable_trbl = unfixable_trouble_count(TRUE);

	/* collect attribute troubles */
	for (idx = 0; idx < A_MAX; idx++) {
	    val_limit = AMAX(idx);
	    /* don't recover strength lost from hunger */
	    if (idx == A_STR && u.uhs >= WEAK) val_limit--;
	    /* don't recover more than 3 points worth of any attribute */
	    if (val_limit > ABASE(idx) + 3) val_limit = ABASE(idx) + 3;

	    for (val = ABASE(idx); val < val_limit; val++)
		attr_trouble(idx);
	    /* keep track of unfixed trouble, for message adjustment below */
	    unfixable_trbl += (AMAX(idx) - val_limit);
	}

	if (trouble_count == 0) {
	    pline(nothing_happens);
	    return;
	} else if (trouble_count > 1) {		/* shuffle */
	    int i, j, k;

	    for (i = trouble_count - 1; i > 0; i--)
		if ((j = rn2(i + 1)) != i) {
		    k = trouble_list[j];
		    trouble_list[j] = trouble_list[i];
		    trouble_list[i] = k;
		}
	}

#if 0	/* Old NetHack success rate */
	/*
	 *		Chances for number of troubles to be fixed
	 *		 0	1      2      3      4	    5	   6	  7
	 *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%	 0.8%
	 *  uncursed:  35.4%  35.4%  22.9%   6.3%    0	    0	   0	  0
	 */
	val_limit = rn2( d(2, (obj && obj->blessed) ? 4 : 2) );
	if (val_limit > trouble_count) val_limit = trouble_count;
#else	/* KMH's new success rate */
	/*
	 * blessed:  Tries all problems, each with chance given below.
	 * uncursed: Tries one problem, with chance given below.
	 * ENCHANT  +0 or less  +1   +2   +3   +4   +5   +6 or more
	 * CHANCE       30%     40%  50%  60%  70%  80%     90%
	 */
	val_limit = (obj && obj->blessed) ? trouble_count : 1;
	if (obj && obj->spe > 0)
		chance = (obj->spe < 6) ? obj->spe+3 : 9;
	else
		chance = 3;
#endif

	/* fix [some of] the troubles */
	for (val = 0; val < val_limit; val++) {
	    idx = trouble_list[val];

		if (rn2(10) < chance)	/* KMH */
	    switch (idx) {
	    case prop2trbl(SICK):
		make_sick(0L, (char *) 0, TRUE, SICK_ALL);
		did_prop++;
		break;
	    case prop2trbl(BLINDED):
		make_blinded((long)u.ucreamed, TRUE);
		did_prop++;
		break;
	    case prop2trbl(HALLUC):
		(void) make_hallucinated(0L, TRUE, 0L);
		did_prop++;
		break;
	    case prop2trbl(VOMITING):
		make_vomiting(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(CONFUSION):
		make_confused(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(STUNNED):
		make_stunned(0L, TRUE);
		did_prop++;
		break;
	    default:
		if (idx >= 0 && idx < A_MAX) {
		    ABASE(idx) += 1;
		    did_attr++;
		} else
		    panic("独角兽角错误！游戏即将崩溃。(%d)", idx);
		break;
	    }
	}

	if (did_attr)
	    pline("这让你变得%s!",
		  (did_prop + did_attr) == (trouble_count + unfixable_trbl) ?
		  "更好了" : "非常好");
	else if (!did_prop)
	    pline("看起来没什么事发生.");

	flags.botl = (did_attr || did_prop);
#undef PROP_COUNT
#undef ATTR_COUNT
#undef prop2trbl
#undef attr2trbl
#undef prop_trouble
#undef attr_trouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *figurine = (struct obj *)arg;
	struct monst *mtmp;
	coord cc;
	boolean cansee_spot, silent, okay_spot;
	boolean redraw = FALSE;
	char monnambuf[BUFSZ], carriedby[BUFSZ];

	if (!figurine) {
#ifdef DEBUG
	    pline("null figurine in fig_transform()");
#endif
	    return;
	}
	silent = (timeout != monstermoves); /* happened while away */
	okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
	if (figurine->where == OBJ_INVENT ||
	    figurine->where == OBJ_MINVENT)
		okay_spot = enexto(&cc, cc.x, cc.y,
				   &mons[figurine->corpsenm]);
	if (!okay_spot ||
	    !figurine_location_checks(figurine,&cc, TRUE)) {
		/* reset the timer to try again later */
		(void) start_timer((long)rnd(5000), TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
		return;
	}

	cansee_spot = cansee(cc.x, cc.y);
	mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
	if (mtmp) {
	    Sprintf(monnambuf, "%s",an(m_monnam(mtmp)));
	    switch (figurine->where) {
		case OBJ_INVENT:
		    if (Blind)
			You_feel("%s从你的背包里头%s!", something,
			    locomotion(mtmp->data,"掉下来了"));
		    else
			You("看见%s从你的背包里头%s!",
			    monnambuf,
			    locomotion(mtmp->data,"掉到地上"));
		    break;

		case OBJ_FLOOR:
		    if (cansee_spot && !silent) {
			You("看见雕像突然变成了%s!",
				monnambuf);
			redraw = TRUE;	/* update figurine's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_spot && !silent) {
			struct monst *mon;
			mon = figurine->ocarry;
			/* figurine carring monster might be invisible */
			if (canseemon(figurine->ocarry)) {
			    Sprintf(carriedby, "%s pack",
				     s_suffix(a_monnam(mon)));
			}
			else if (is_pool(mon->mx, mon->my))
			    Strcpy(carriedby, "空荡荡的水池");
			else
			    Strcpy(carriedby, "稀薄的空气");
			You("see %s %s out of %s!", monnambuf,
			    locomotion(mtmp->data, "drop"), carriedby);
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif

		default:
		    impossible("figurine came to life where? (%d)",
				(int)figurine->where);
		break;
	    }
	}
	/* free figurine now */
	obj_extract_self(figurine);
	obfree(figurine, (struct obj *)0);
	if (redraw) newsym(cc.x, cc.y);
}

STATIC_OVL boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
	xchar x,y;

	if (carried(obj) && u.uswallow) {
		if (!quietly)
			You("在这没有足够的空间.");
		return FALSE;
	}
	x = cc->x; y = cc->y;
	if (!isok(x,y)) {
		if (!quietly)
			You("没法把小雕像放在这里.");
		return FALSE;
	}
	if (IS_ROCK(levl[x][y].typ) &&
	    !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x,y))) {
		if (!quietly)
		    You("没法把小雕像放在%s!",
			IS_TREE(levl[x][y].typ) ? "树里头" : "墙壁里头");
		return FALSE;
	}
	if (sobj_at(BOULDER,x,y) && !passes_walls(&mons[obj->corpsenm])
			&& !throws_rocks(&mons[obj->corpsenm])) {
		if (!quietly)
			You("没法把小雕像塞到巨石里头.");
		return FALSE;
	}
	return TRUE;
}

STATIC_OVL void
use_figurine(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	xchar x, y;
	coord cc;

	if (u.uswallow) {
		/* can't activate a figurine while swallowed */
		if (!figurine_location_checks(obj, (coord *)0, FALSE))
			return;
	}
	if(!getdir((char *)0)) {
		flags.move = multi = 0;
		return;
	}
	x = u.ux + u.dx; y = u.uy + u.dy;
	cc.x = x; cc.y = y;
	/* Passing FALSE arg here will result in messages displayed */
	if (!figurine_location_checks(obj, &cc, FALSE)) return;
	You("%s然后它变成了生物.",
	    (u.dx||u.dy) ? "把小雕像放在你背后" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ||
	     is_pool(cc.x, cc.y)) ?
		"把小雕像放了出去" :
	    (u.dz < 0 ?
		"把小雕像抛向空中" :
		"把小雕像放在地上"));
	(void) make_familiar(obj, cc.x, cc.y, FALSE);
	(void) stop_timer(FIG_TRANSFORM, (genericptr_t)obj);
	useup(obj);
	*optr = 0;
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };
static NEARDATA const char need_to_remove_outer_armor[] =
			"你得先把你的%s脱掉才能给你的%s涂上油脂.";

STATIC_OVL void
use_grease(obj)
struct obj *obj;
{
	struct obj *otmp;
	char buf[BUFSZ];

	if (Glib) {
	    pline("%s从你的%s.", Tobjnam(obj, "滑落"),
		  makeplural(body_part(FINGER)));
	    dropx(obj);
	    return;
	}

	if (obj->spe > 0) {
		if ((obj->cursed || Fumbling) && !rn2(2)) {
			consume_obj_charge(obj, TRUE);

			pline("%s从你的%s.", Tobjnam(obj, "滑落"),
			      makeplural(body_part(FINGER)));
			dropx(obj);
			return;
		}
		otmp = getobj(lubricables, "grease");
		if (!otmp) return;
		if ((otmp->owornmask & WORN_ARMOR) && uarmc) {
			Strcpy(buf, xname(uarmc));
			You(need_to_remove_outer_armor, buf, xname(otmp));
			return;
		}
#ifdef TOURIST
		if ((otmp->owornmask & WORN_SHIRT) && (uarmc || uarm)) {
			Strcpy(buf, uarmc ? xname(uarmc) : "");
			if (uarmc && uarm) Strcat(buf, " and ");
			Strcat(buf, uarm ? xname(uarm) : "");
			You(need_to_remove_outer_armor, buf, xname(otmp));
			return;
		}
#endif
		consume_obj_charge(obj, TRUE);

		if (otmp != &zeroobj) {
			You("往%s上涂了厚厚的一层润滑油.",
			    yname(otmp));
			otmp->greased = 1;
			if (obj->cursed && !nohands(youmonst.data)) {
			    incr_itimeout(&Glib, rnd(15));
			    pline("你一不小心把满%s都搞满了润滑油.",
				makeplural(body_part(HAND)));
			}
		} else {
			Glib += rnd(15);
			You("往你的%s上涂满了油脂.",
			    makeplural(body_part(FINGER)));
		}
	} else {
	    if (obj->known)
		pline("%s用完了.", Tobjnam(obj, "已经"));
	    else
		pline("%s已经空了.", Tobjnam(obj, "看起来"));
	}
	update_inventory();
}

static struct trapinfo {
	struct obj *tobj;
	xchar tx, ty;
	int time_needed;
	boolean force_bungle;
} trapinfo;

void
reset_trapset()
{
	trapinfo.tobj = 0;
	trapinfo.force_bungle = 0;
}

static struct whetstoneinfo {
	struct obj *tobj, *wsobj;
	int time_needed;
} whetstoneinfo;

void
reset_whetstone()
{
	whetstoneinfo.tobj = 0;
	whetstoneinfo.wsobj = 0;
}

/* occupation callback */
STATIC_PTR
int
set_whetstone()
{
	struct obj *otmp = whetstoneinfo.tobj, *ows = whetstoneinfo.wsobj;
	int chance;

	if (!otmp || !ows) {
	    reset_whetstone();
	    return 0;
	} else
	if (!carried(otmp) || !carried(ows)) {
	    You("看起来用%s的方法错了.",
		!carried(otmp) ? yname(otmp) : yname(ows));
	    reset_whetstone();
	    return 0;
	}

	if (--whetstoneinfo.time_needed > 0) {
	    int adj = 2;
	    if (Blind) adj--;
	    if (Fumbling) adj--;
	    if (Confusion) adj--;
	    if (Stunned) adj--;
	    if (Hallucination) adj--;
	    if (adj > 0)
		whetstoneinfo.time_needed -= adj;
	    return 1;
	}

	chance = 4 - (ows->blessed) + (ows->cursed*2) + (otmp->oartifact ? 3 : 0);

	if (!rn2(chance) && (ows->otyp == WHETSTONE)) {
	    /* Remove rust first, then sharpen dull edges */
	    if (otmp->oeroded) {
		otmp->oeroded--;
		pline("%s %s%s了.", Yname2(otmp),
		    (Blind ? "估计是 " : (otmp->oeroded ? "已经 " : "")),
		    otense(otmp, "寒光闪闪的"));
	    } else
	    if (otmp->spe < 0) {
		otmp->spe++;
		pline("%s %s %s锋利了.%s", Yname2(otmp),
		    otense(otmp, Blind ? "感觉" : "看起来"),
		    (otmp->spe >= 0 ? "更 " : ""),
		    Blind ? "  (哎呀。你划到了自己的手)" : "");
	    }
	    makeknown(WHETSTONE);
	    reset_whetstone();
	} else {
	    if (Hallucination)
		pline("%s都是%s的错!",
		    is_plural(ows) ? "这" : "这些", xname(ows));
	    else pline("%s", Blind ? "哎呦，真是麻烦的活." :
		"你磨了半天，总算看不见任何视觉上的改变了.");
	    reset_whetstone();
	}

	return 0;
}

/* use stone on obj. the stone doesn't necessarily need to be a whetstone. */
STATIC_OVL void
use_whetstone(stone, obj)
struct obj *stone, *obj;
{
	boolean fail_use = TRUE;
	const char *occutext = "sharpening";
	int tmptime = 130 + (rnl(13) * 5);

	if (u.ustuck && sticks(youmonst.data)) {
	    You("至少应该先摆脱%s.", mon_nam(u.ustuck));
	} else
	if ((welded(uwep) && (uwep != stone)) ||
		(uswapwep && u.twoweap && welded(uswapwep) && (uswapwep != obj))) {
	    You("你至少得有个空手吧?");
	} else
	if (nohands(youmonst.data)) {
	    You("你试图拿着%s的%s结构不存在.",
		an(xname(stone)), makeplural(body_part(HAND)));
	} else
	if (verysmall(youmonst.data)) {
	    You("在如此小的体型下估计用一会%s就得被活活累死.", an(xname(stone)));
	} else
#ifdef GOLDOBJ
	if (obj == &goldobj) {
	    pline("店主会很快注意到你身上闪闪发光的金币%s的.",
		obj->quan > 1 ? "们" : "");
	} else
#endif
	if (!is_pool(u.ux, u.uy) && !IS_FOUNTAIN(levl[u.ux][u.uy].typ)
#ifdef SINKS
	    && !IS_SINK(levl[u.ux][u.uy].typ) && !IS_TOILET(levl[u.ux][u.uy].typ)
#endif
	    ) {
	    if (carrying(POT_WATER) && objects[POT_WATER].oc_name_known) {
		pline("你最好别这么浪费水资源.");
	    } else
		You("使用这个的时候需要一些水.");
	} else
	if (Levitation && !Lev_at_will && !u.uinwater) {
	    You("根本没法碰到水面.");
	} else
	    fail_use = FALSE;

	if (fail_use) {
	    reset_whetstone();
	    return;
	}

	if (stone == whetstoneinfo.wsobj && obj == whetstoneinfo.tobj &&
	    carried(obj) && carried(stone)) {
	    You("接着%s %s.", occutext, yname(obj));
	    set_occupation(set_whetstone, occutext, 0);
	    return;
	}

	if (obj) {
	    int ttyp = obj->otyp;
	    boolean isweapon = (obj->oclass == WEAPON_CLASS || is_weptool(obj));
	    boolean isedged = (is_pick(obj) ||
				(objects[ttyp].oc_dir & (PIERCE|SLASH)));
	    if (obj == &zeroobj) {
		You("开始磨你的钉子.");
	    } else
	    if (!isweapon || !isedged) {
		pline("%s足够锋利了.",
			is_plural(obj) ? "已经" : "已经");
	    } else
	    if (stone->quan > 1) {
		pline("一次只用一个%s会更轻松.", singular(stone, xname));
	    } else
	    if (obj->quan > 1) {
		You("只能一次在%s上磨一个%s.",
		    the(xname(stone)),
		    (obj->oclass == WEAPON_CLASS ? "武器" : "物品"));
	    } else
	    if (!is_metallic(obj)) {
		pline("这会把%s %s毁了的.",
			materialnm[objects[ttyp].oc_material],
		xname(obj));
	    } else
	    if (((obj->spe >= 0) || !obj->known) && !obj->oeroded) {
		pline("%s %s已经足够的锋利了.",
			is_plural(obj) ? "它们" : "它",
			otense(obj, Blind ? "感觉上" : "看起来"));
	    } else {
		if (stone->cursed) tmptime *= 2;
		whetstoneinfo.time_needed = tmptime;
		whetstoneinfo.tobj = obj;
		whetstoneinfo.wsobj = stone;
		You("开始%s %s.", occutext, yname(obj));
		set_occupation(set_whetstone, occutext, 0);
		if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)) whetstone_fountain_effects(obj);
#ifdef SINKS
		else if (IS_SINK(levl[u.ux][u.uy].typ)) whetstone_sink_effects(obj);
		else if (IS_TOILET(levl[u.ux][u.uy].typ)) whetstone_toilet_effects(obj);
#endif
	    }
	} else You("把%s在%s中挥舞.", the(xname(stone)),
	    (IS_POOL(levl[u.ux][u.uy].typ) && Underwater) ? "水" : "空气");
}

/* touchstones - by Ken Arnold */
STATIC_OVL void
use_stone(tstone)
struct obj *tstone;
{
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color, *choices;
    char stonebuf[QBUFSZ];
    static const char scritch[] = "\"咔嚓，咔嚓\"";
    static const char allowall[3] = { COIN_CLASS, ALL_CLASSES, 0 };
    static const char justgems[3] = { ALLOW_NONE, GEM_CLASS, 0 };
#ifndef GOLDOBJ
    struct obj goldobj;
#endif

    /* in case it was acquired while blinded */
    if (!Blind) tstone->dknown = 1;
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    choices = (tstone->otyp == TOUCHSTONE && tstone->dknown &&
		objects[TOUCHSTONE].oc_name_known) ? justgems : allowall;
    Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan));
    if ((obj = getobj(choices, stonebuf)) == 0)
	return;
#ifndef GOLDOBJ
    if (obj->oclass == COIN_CLASS) {
	u.ugold += obj->quan;	/* keep botl up to date */
	goldobj = *obj;
	dealloc_obj(obj);
	obj = &goldobj;
    }
#endif

    if (obj == tstone && obj->quan == 1) {
	You_cant("在%s上面磨它自己.", the(xname(obj)));
	return;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed &&
	    obj->oclass == GEM_CLASS && !is_graystone(obj) &&
	    !obj_resists(obj, 80, 100)) {
	if (Blind)
	    pline("你感觉有什么东西变得更尖锐了.");
	else if (Hallucination)
	    pline("哎呀，哎呀，看看这美丽的碎片.");
	else
	    pline("一个锋利的裂缝在%s%s上延展开来.",
		  (obj->quan > 1) ? "其中一个 " : "", the(xname(obj)));
#ifndef GOLDOBJ
     /* assert(obj != &goldobj); */
#endif
	useup(obj);
	return;
    }

    if (Blind) {
	pline(scritch);
	return;
    } else if (Hallucination) {
	pline("我去，交响乐队!");
	return;
    }

    do_scratch = FALSE;
    streak_color = 0;

    switch (obj->oclass) {
    case WEAPON_CLASS:
    case TOOL_CLASS:
	use_whetstone(tstone, obj);
	return;
    case GEM_CLASS:	/* these have class-specific handling below */
    case RING_CLASS:
	if (tstone->otyp != TOUCHSTONE) {
	    do_scratch = TRUE;
	} else if (obj->oclass == GEM_CLASS && (tstone->blessed ||
		(!tstone->cursed &&
		    (Role_if(PM_ARCHEOLOGIST) || Race_if(PM_GNOME))))) {
	    makeknown(TOUCHSTONE);
	    makeknown(obj->otyp);
	    prinv((char *)0, obj, 0L);
	    return;
	} else {
	    /* either a ring or the touchstone was not effective */
	    if (objects[obj->otyp].oc_material == GLASS) {
		do_scratch = TRUE;
		break;
	    }
	}
	streak_color = c_obj_colors[objects[obj->otyp].oc_color];
	break;		/* gem or ring */

    default:
	switch (objects[obj->otyp].oc_material) {
	case CLOTH:
	    pline("%s被稍微地抛光了一下.", Tobjnam(tstone, "看起来"));
	    return;
	case LIQUID:
	    if (!obj->known)		/* note: not "whetstone" */
		You("这玩意可不是什么吸水石头.");
	    else
		pline("%s变得湿了.", Tobjnam(tstone, "are"));
	    return;
	case WAX:
	    streak_color = "蜡色的";
	    break;		/* okay even if not touchstone */
	case WOOD:
	    streak_color = "木屑";
	    break;		/* okay even if not touchstone */
	case GOLD:
	    do_scratch = TRUE;	/* scratching and streaks */
	    streak_color = "金色";
	    break;
	case SILVER:
	    do_scratch = TRUE;	/* scratching and streaks */
	    streak_color = "银色"
                       "";
	    break;
	default:
	    /* Objects passing the is_flimsy() test will not
	       scratch a stone.  They will leave streaks on
	       non-touchstones and touchstones alike. */
	    if (is_flimsy(obj))
		streak_color = c_obj_colors[objects[obj->otyp].oc_color];
	    else
		do_scratch = (tstone->otyp != TOUCHSTONE);
	    break;
	}
	break;		/* default oclass */
    }

    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
    if (do_scratch)
	pline("你在%s上搞出%s的痕迹。",
	      streak_color ? streak_color : (const char *)"",
	      streak_color ? " " : "", stonebuf);
    else if (streak_color)
	pline("You see %s streaks on the %s.", streak_color, stonebuf);
    else
	pline(scritch);
    return;
}

/* Place a landmine/bear trap.  Helge Hafting */
STATIC_OVL void
use_trap(otmp)
struct obj *otmp;
{
	int ttyp, tmp;
	const char *what = (char *)0;
	char buf[BUFSZ];
	const char *occutext = "设置陷阱";

	if (nohands(youmonst.data))
	    what = "没有手的情况下";
	else if (Stunned)
	    what = "混乱的时候";
	else if (u.uswallow)
	    what = is_animal(u.ustuck->data) ? "被吃进去的时候" :
			"被吞进去的时候";
	else if (Underwater)
	    what = "水下";
	else if (Levitation)
	    what = "漂浮的时候";
	else if (is_pool(u.ux, u.uy))
	    what = "水里";
	else if (is_lava(u.ux, u.uy))
	    what = "岩浆里";
	else if (On_stairs(u.ux, u.uy))
	    what = (u.ux == xdnladder || u.ux == xupladder) ?
			"梯子上" : "楼梯上";
	else if (IS_FURNITURE(levl[u.ux][u.uy].typ) ||
		IS_ROCK(levl[u.ux][u.uy].typ) ||
		closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
	    what = "这里";
	if (what) {
	    You_cant("在%s的情况下设置陷阱!",what);
	    reset_trapset();
	    return;
	}
	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	if (otmp == trapinfo.tobj &&
		u.ux == trapinfo.tx && u.uy == trapinfo.ty) {
	    You("继续%s %s.",
		shk_your(buf, otmp),
		defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
	    set_occupation(set_trap, occutext, 0);
	    return;
	}
	trapinfo.tobj = otmp;
	trapinfo.tx = u.ux,  trapinfo.ty = u.uy;
	tmp = ACURR(A_DEX);
	trapinfo.time_needed = (tmp > 17) ? 2 : (tmp > 12) ? 3 :
				(tmp > 7) ? 4 : 5;
	if (Blind) trapinfo.time_needed *= 2;
	tmp = ACURR(A_STR);
	if (ttyp == BEAR_TRAP && tmp < 18)
	    trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
	/*[fumbling and/or confusion and/or cursed object check(s)
	   should be incorporated here instead of in set_trap]*/
#ifdef STEED
	if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
	    boolean chance;

	    if (Fumbling || otmp->cursed) chance = (rnl(10) > 3);
	    else  chance = (rnl(10) > 5);
	    You("你对于设置%s这件事没有太多的经验.",
		mon_nam(u.usteed));
	    Sprintf(buf, "仍旧继续设置陷阱吗%s?",
		the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
	    if(yn(buf) == 'y') {
		if (chance) {
			switch(ttyp) {
			    case LANDMINE:	/* set it off */
			    	trapinfo.time_needed = 0;
			    	trapinfo.force_bungle = TRUE;
				break;
			    case BEAR_TRAP:	/* drop it without arming it */
				reset_trapset();
				You("手忙脚乱的把%s丢掉了!",
			  the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
				dropx(otmp);
				return;
			}
		}
	    } else {
	    	reset_trapset();
		return;
	    }
	}
#endif
	You("开始设置%s %s.",
	    shk_your(buf, otmp),
	    defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
	set_occupation(set_trap, occutext, 0);
	return;
}

STATIC_PTR
int
set_trap()
{
	struct obj *otmp = trapinfo.tobj;
	struct trap *ttmp;
	int ttyp;

	if (!otmp || !carried(otmp) ||
		u.ux != trapinfo.tx || u.uy != trapinfo.ty) {
	    /* ?? */
	    reset_trapset();
	    return 0;
	}

	if (--trapinfo.time_needed > 0) return 1;	/* still busy */

	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	ttmp = maketrap(u.ux, u.uy, ttyp);
	if (ttmp) {
	    ttmp->tseen = 1;
	    ttmp->madeby_u = 1;
	    newsym(u.ux, u.uy); /* if our hero happens to be invisible */
	    if (*in_rooms(u.ux,u.uy,SHOPBASE)) {
		add_damage(u.ux, u.uy, 0L);		/* schedule removal */
	    }
	    if (!trapinfo.force_bungle)
		You("成功的把%s设置好了.",
			the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
	    if (((otmp->cursed || Fumbling) && (rnl(10) > 5)) || trapinfo.force_bungle)
		dotrap(ttmp,
			(unsigned)(trapinfo.force_bungle ? FORCEBUNGLE : 0));
	} else {
	    /* this shouldn't happen */
	    Your("的设置陷阱操作失败了.");
	}
	useup(otmp);
	reset_trapset();
	return 0;
}

STATIC_OVL int
use_whip(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = 0;
    const char *msg_slipsfree = "牛鞭没打中.";
    const char *msg_snap = "啪!";

    if (obj != uwep) {
	if (!wield_tool(obj, "lash")) return 0;
	else res = 1;
    }
    if (!getdir((char *)0)) return res;

    if (Stunned || (Confusion && !rn2(5))) confdir();
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    mtmp = m_at(rx, ry);

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST)) ++proficient;
    if (ACURR(A_DEX) < 6) proficient--;
    else if (ACURR(A_DEX) >= 14) proficient += (ACURR(A_DEX) - 14);
    if (Fumbling) --proficient;
    if (proficient > 3) proficient = 3;
    if (proficient < 0) proficient = 0;

    if (u.uswallow && attack(u.ustuck)) {
	There("对于你挥舞牛鞭来说空间太小了.");

    } else if (Underwater) {
	There("对于你挥舞牛鞭来说阻力太大了.");

    } else if (u.dz < 0) {
	You("从%s上打下来一只臭虫.",ceiling(u.ux,u.uy));

    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
	int dam;

#ifdef STEED
	/* Sometimes you hit your steed by mistake */
	if (u.usteed && !rn2(proficient + 2)) {
	    You("抽打%s!", mon_nam(u.usteed));
	    kick_steed();
	    return 1;
	}
#endif
	if (Levitation
#ifdef STEED
			|| u.usteed
#endif
		) {
	    /* Have a shot at snaring something on the floor */
	    otmp = level.objects[u.ux][u.uy];
	    if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
		pline("你是想要死马当活马医吗?");
		return 1;
	    }
	    if (otmp && proficient) {
		You("用你的牛鞭缠绕住%s，它正躺在%s那边.",
		    an(singular(otmp, xname)), surface(u.ux, u.uy));
		if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
		    pline(msg_slipsfree);
		return 1;
	    }
	}
	dam = rnd(2) + dbon() + obj->spe;
	if (dam <= 0) dam = 1;
	You("用你的牛鞭抽打你的%s.", body_part(FOOT));
	Sprintf(buf, "被%s的胡乱挥舞的牛鞭杀死", uhim());
	losehp(dam, buf, NO_KILLER_PREFIX);
	flags.botl = 1;
	return 1;

    } else if ((Fumbling || Glib) && !rn2(5)) {
	pline_The("牛鞭从你颤抖着的%s上滑落.", body_part(HAND));
	dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
	/*
	 *     Assumptions:
	 *
	 *	if you're in a pit
	 *		- you are attempting to get out of the pit
	 *		- or, if you are applying it towards a small
	 *		  monster then it is assumed that you are
	 *		  trying to hit it.
	 *	else if the monster is wielding a weapon
	 *		- you are attempting to disarm a monster
	 *	else
	 *		- you are attempting to hit the monster
	 *
	 *	if you're confused (and thus off the mark)
	 *		- you only end up hitting.
	 *
	 */
	const char *wrapped_what = (char *)0;

	if (mtmp) {
	    if (bigmonst(mtmp->data)) {
		wrapped_what = strcpy(buf, mon_nam(mtmp));
	    } else if (proficient) {
		if (attack(mtmp)) return 1;
		else pline(msg_snap);
	    }
	}
	if (!wrapped_what) {
	    if (IS_FURNITURE(levl[rx][ry].typ))
		wrapped_what = something;
	    else if (sobj_at(BOULDER, rx, ry))
		wrapped_what = "一个巨石";
	}
	if (wrapped_what) {
	    coord cc;

	    cc.x = rx; cc.y = ry;
	    You("用你的牛鞭缠绕住%s.", wrapped_what);
	    if (proficient && rn2(proficient + 2)) {
		if (!mtmp || enexto(&cc, rx, ry, youmonst.data)) {
		    You("把你自己拉出了坑!");
		    teleds(cc.x, cc.y, TRUE);
		    u.utrap = 0;
		    vision_full_recalc = 1;
		}
	    } else {
		pline(msg_slipsfree);
	    }
	    if (mtmp) wakeup(mtmp);
	} else pline(msg_snap);

    } else if (mtmp) {
	if (!canspotmon(mtmp) &&
		!memory_is_invisible(rx, ry)) {
	   pline("这边的怪物你看不到.");
	   map_invisible(rx, ry);
	}
	otmp = MON_WEP(mtmp);	/* can be null */
	if (otmp) {
	    char onambuf[BUFSZ];
	    const char *mon_hand;
	    boolean gotit = proficient && (!Fumbling || !rn2(10));

	    Strcpy(onambuf, cxname(otmp));
	    if (gotit) {
		mon_hand = mbodypart(mtmp, HAND);
		if (bimanual(otmp)) mon_hand = makeplural(mon_hand);
	    } else
		mon_hand = 0;	/* lint suppression */

	    You("用你的牛鞭抽打%s并缠上了%s.",
		s_suffix(mon_nam(mtmp)), onambuf);
	    if (gotit && otmp->cursed) {
		pline("%s被%s的%s死死的握住了%c",
		      (otmp->quan == 1L) ? "它看起来" : "它们看起来",
		      mhis(mtmp), mon_hand,
		      !otmp->bknown ? '!' : '.');
		otmp->bknown = 1;
		gotit = FALSE;	/* can't pull it free */
	    }
	    if (gotit) {
		obj_extract_self(otmp);
		possibly_unwield(mtmp, FALSE);
		setmnotwielded(mtmp,otmp);

		switch (rn2(proficient + 1)) {
		case 2:
		    /* to floor near you */
		    You("yank %s %s to the %s!", s_suffix(mon_nam(mtmp)),
			onambuf, surface(u.ux, u.uy));
		    place_object(otmp, u.ux, u.uy);
		    stackobj(otmp);
		    break;
		case 3:
		    /* right to you */
#if 0
		    if (!rn2(25)) {
			/* proficient with whip, but maybe not
			   so proficient at catching weapons */
			int hitu, hitvalu;

			hitvalu = 8 + otmp->spe;
			hitu = thitu(hitvalu,
				     dmgval(otmp, &youmonst),
				     otmp, (char *)0);
			if (hitu) {
			    pline_The("%s在你准备用皮鞭抽落它的武器时握住了它!",
				the(onambuf));
			}
			place_object(otmp, u.ux, u.uy);
			stackobj(otmp);
			break;
		    }
#endif /* 0 */
		    /* right into your inventory */
		    You("准确无误的抓住了%s的%s!", s_suffix(mon_nam(mtmp)), onambuf);
		    if (otmp->otyp == CORPSE &&
			    touch_petrifies(&mons[otmp->corpsenm]) &&
			    !uarmg && !Stone_resistance &&
			    !(poly_when_stoned(youmonst.data) &&
				polymon(PM_STONE_GOLEM))) {
			char kbuf[BUFSZ];

			Sprintf(kbuf, "%s的尸体",
				an(mons[otmp->corpsenm].mname));
			pline("你拿牛鞭抢%s的时候难道就不会思考一下吗？", kbuf);
			instapetrify(kbuf);
		    }
		    otmp = hold_another_object(otmp, "丢下了%s!",
					       doname(otmp), (const char *)0);
		    break;
		default:
		    /* to floor beneath mon */
		    You("把%s从%s里头%s!", the(onambuf),
			s_suffix(mon_nam(mtmp)), mon_hand);
		    obj_no_longer_held(otmp);
		    place_object(otmp, mtmp->mx, mtmp->my);
		    stackobj(otmp);
		    break;
		}
	    } else {
		pline(msg_slipsfree);
	    }
	    wakeup(mtmp);
	} else {
	    if (mtmp->m_ap_type &&
		!Protection_from_shape_changers && !sensemon(mtmp))
		stumble_onto_mimic(mtmp);
	    else You("朝着%s轻轻挥舞鞭子.", mon_nam(mtmp));
	    if (proficient) {
		if (attack(mtmp)) return 1;
		else pline(msg_snap);
	    }
	}

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
	    /* it must be air -- water checked above */
	    You("在半空中胡乱挥舞鞭子.");

    } else {
	pline(msg_snap);

    }
    return 1;
}


static const char
	not_enough_room[] = "没有足够的空间让你挥舞鞭子！",
	where_to_hit[] = "你想抽哪里?",
	cant_see_spot[] = "在看不清那边的情况下你打不中东西的.",
	cant_reach[] = "没法碰到那边.";

/* Distance attacks by pole-weapons */
STATIC_OVL int
use_pole (obj)
	struct obj *obj;
{
	int res = 0, typ, max_range;
	int min_range = obj->otyp == FISHING_POLE ? 1 : 4;
	coord cc;
	struct monst *mtmp;
	struct obj *otmp;
	boolean fishing;


	/* Are you allowed to use the pole? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj, "swing")) return(0);
	    else res = 1;
	}

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, "准备击打的目标") < 0)
	    return 0;	/* user pressed ESC */

#ifdef WEAPON_SKILLS
	/* Calculate range */
	typ = weapon_type(obj);
	if (typ == P_NONE || P_SKILL(typ) <= P_BASIC) max_range = 4;
	else if (P_SKILL(typ) <= P_SKILLED) max_range = 5;
	else max_range = 8;
#else
	max_range = 8;
#endif

	if (distu(cc.x, cc.y) > max_range) {
	    pline("你选的地方太远了!");
	    return (res);
	} else if (distu(cc.x, cc.y) < min_range) {
	    pline("太近了!");
	    return (res);
	} else if (!cansee(cc.x, cc.y) &&
		   ((mtmp = m_at(cc.x, cc.y)) == (struct monst *)0 ||
		    !canseemon(mtmp))) {
	    You(cant_see_spot);
	    return (res);
	} else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
	    You(cant_reach);
	    return res;
	}

	/* What is there? */
	mtmp = m_at(cc.x, cc.y);

	if (obj->otyp == FISHING_POLE) {
	    fishing = is_pool(cc.x, cc.y);
	    /* Try a random effect */
	    switch (rnd(6))
	    {
		case 1:
		    /* Snag yourself */
		    You("把你自己的手指头钩住了!");
		    losehp(rn1(10,10), "钓鱼竿上面的钩子", KILLED_BY);
		    return 1;
		case 2:
		    /* Reel in a fish */
		    if (mtmp) {
			if ((bigmonst(mtmp->data) || strongmonst(mtmp->data))
				&& !rn2(2)) {
			    You("反而被%s扯了过去！", surface(cc.x,cc.y));
			    hurtle(sgn(cc.x-u.ux), sgn(cc.y-u.uy), 1, TRUE);
			    return 1;
			} else if (enexto(&cc, u.ux, u.uy, 0)) {
			    You("把%s钩了过来!", mon_nam(mtmp));
			    mtmp->mundetected = 0;
			    rloc_to(mtmp, cc.x, cc.y);
			    return 1;
			}
		    }
		    break;
		case 3:
		    /* Snag an existing object */
		    if ((otmp = level.objects[cc.x][cc.y]) != (struct obj *)0) {
			You("从%s那里钓到了一个东西!", surface(cc.x, cc.y));
			pickup_object(otmp, 1, FALSE);
			/* If pickup fails, leave it alone */
			newsym(cc.x, cc.y);
			return 1;
		    }
		    break;
		case 4:
		    /* Snag some garbage */
		    if (fishing && flags.boot_count < 1 &&
			    (otmp = mksobj(LOW_BOOTS, TRUE, FALSE)) !=
			    (struct obj *)0) {
			flags.boot_count++;
			You("从%s那里钓到了点垃圾!",
				surface(cc.x, cc.y));
			if (pickup_object(otmp, 1, FALSE) <= 0) {
			    obj_extract_self(otmp);
			    place_object(otmp, u.ux, u.uy);
			    newsym(u.ux, u.uy);
			}
			return 1;
		    }
#ifdef SINKS
		    /* Or a rat in the sink/toilet */
		    if (!(mvitals[PM_SEWER_RAT].mvflags & G_GONE) &&
			    (IS_SINK(levl[cc.x][cc.y].typ) ||
			    IS_TOILET(levl[cc.x][cc.y].typ))) {
			mtmp = makemon(&mons[PM_SEWER_RAT], cc.x, cc.y,
				NO_MM_FLAGS);
			pline("哎呀！那边有个%s!",
				Blind ? "很可怕的东西" : a_monnam(mtmp));
			return 1;
		    }
#endif
		    break;
		case 5:
		    /* Catch your dinner */
		    if (fishing && (otmp = mksobj(CRAM_RATION, TRUE, FALSE)) !=
			    (struct obj *)0) {
			You("钓到了今天晚上的晚饭!");
			if (pickup_object(otmp, 1, FALSE) <= 0) {
			    obj_extract_self(otmp);
			    place_object(otmp, u.ux, u.uy);
			    newsym(u.ux, u.uy);
			}
			return 1;
		    }
		    break;
		default:
		case 6:
		    /* Untrap */
		    /* FIXME -- needs to deal with non-adjacent traps */
		    break;
	    }
	}

	/* The effect didn't apply.  Attack the monster there. */
	if (mtmp) {
	    int oldhp = mtmp->mhp;

	    bhitpos = cc;
	    check_caitiff(mtmp);
	    (void) thitmonst(mtmp, uwep, 1);
	    /* check the monster's HP because thitmonst() doesn't return
	     * an indication of whether it hit.  Not perfect (what if it's a
	     * non-silver weapon on a shade?)
	     */
	    if (mtmp->mhp < oldhp)
		u.uconduct.weaphit++;
	} else
	    /* Now you know that nothing is there... */
	    pline(nothing_happens);
	return (1);
}

STATIC_OVL int
use_cream_pie(obj)
struct obj *obj;
{
	boolean wasblind = Blind;
	boolean wascreamed = u.ucreamed;
	boolean several = FALSE;

	if (obj->quan > 1L) {
		several = TRUE;
		obj = splitobj(obj, 1L);
	}
	if (Hallucination)
		You("给你自己做了个面膜.");
	else
		pline("你把你的%s泡进%s%s.", body_part(FACE),
			several ? "一个" : "",
			several ? makeplural(the(xname(obj))) : the(xname(obj)));
	if(can_blnd((struct monst*)0, &youmonst, AT_WEAP, obj)) {
		int blindinc = rnd(25);
		u.ucreamed += blindinc;
		make_blinded(Blinded + (long)blindinc, FALSE);
		if (!Blind || (Blind && wasblind))
			pline("现在有%s黏糊糊的玩意黏在你的%s.",
				wascreamed ? "更多的 " : "",
				body_part(FACE));
		else /* Blind  && !wasblind */
			You_cant("透过糊在你%s上的那一坨黏糊糊的玩意看到任何东西.",
				body_part(FACE));
	}
	if (obj->unpaid) {
		verbalize("你既然用了它，你就必须买下它!");
		bill_dummy_object(obj);
	}
	obj_extract_self(obj);
	delobj(obj);
	return(0);
}

STATIC_OVL int
use_grapple (obj)
	struct obj *obj;
{
	int res = 0, typ, max_range = 4, tohit;
	coord cc;
	struct monst *mtmp;
	struct obj *otmp;

	/* Are you allowed to use the hook? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj, "cast")) return(0);
	    else res = 1;
	}
     /* assert(obj == uwep); */

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, "准备击打的目标") < 0)
	    return 0;	/* user pressed ESC */

	/* Calculate range */
	typ = uwep_skill_type();
	if (typ == P_NONE || P_SKILL(typ) <= P_BASIC) max_range = 4;
	else if (P_SKILL(typ) == P_SKILLED) max_range = 5;
	else max_range = 8;
	if (distu(cc.x, cc.y) > max_range) {
		pline("太远了!");
		return (res);
	} else if (!cansee(cc.x, cc.y)) {
		You(cant_see_spot);
		return (res);
	}

	/* What do you want to hit? */
	tohit = rn2(5);
	if (typ != P_NONE && P_SKILL(typ) >= P_SKILLED) {
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    anything any;
	    char buf[BUFSZ];
	    menu_item *selected;

	    any.a_void = 0;	/* set all bits to zero */
	    any.a_int = 1;	/* use index+1 (cant use 0) as identifier */
	    start_menu(tmpwin);
	    any.a_int++;
	    Sprintf(buf, "一个在%s上的东西", surface(cc.x, cc.y));
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	    any.a_int++;
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			"一个怪物", MENU_UNSELECTED);
	    any.a_int++;
	    Sprintf(buf, "%s", surface(cc.x, cc.y));
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	    end_menu(tmpwin, "瞄准哪个?");
	    tohit = rn2(4);
	    if (select_menu(tmpwin, PICK_ONE, &selected) > 0 &&
			rn2(P_SKILL(typ) > P_SKILLED ? 20 : 2))
		tohit = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    destroy_nhwindow(tmpwin);
	}

	/* What did you hit? */
	switch (tohit) {
	case 0:	/* Trap */
	    /* FIXME -- untrap needs to deal with non-adjacent traps */
	    break;
	case 1:	/* Object */
	    if ((otmp = level.objects[cc.x][cc.y]) != 0) {
		You("从%s上钓上一个物品!", surface(cc.x, cc.y));
		(void) pickup_object(otmp, 1L, FALSE);
		/* If pickup fails, leave it alone */
		newsym(cc.x, cc.y);
		return (1);
	    }
	    break;
	case 2:	/* Monster */
	    if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *)0) break;
	    if (verysmall(mtmp->data) && !rn2(4) &&
			enexto(&cc, u.ux, u.uy, (struct permonst *)0)) {
		You("把%s拽了出来!", mon_nam(mtmp));
		mtmp->mundetected = 0;
		rloc_to(mtmp, cc.x, cc.y);
		return (1);
	    } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data)) ||
		       rn2(4)) {
		(void) thitmonst(mtmp, uwep, 1);
		return (1);
	    }
	    /* FALL THROUGH */
	case 3:	/* Surface */
	    if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
		pline_The("钩子从%s上滑落了.", surface(cc.x, cc.y));
	    else {
		You("你被扯到%s处!", surface(cc.x, cc.y));
		hurtle(sgn(cc.x-u.ux), sgn(cc.y-u.uy), 1, FALSE);
		spoteffects(TRUE);
	    }
	    return (1);
	default:	/* Yourself (oops!) */
	    if (P_SKILL(typ) <= P_BASIC) {
		You("钩住了你自己!");
		losehp(rn1(10,10), "钩绳", KILLED_BY);
		return (1);
	    }
	    break;
	}
	pline(nothing_happens);
	return (1);
}


#define BY_OBJECT	((struct monst *)0)

/* return 1 if the wand is broken, hence some time elapsed */
STATIC_OVL int
do_break_wand(obj)
    struct obj *obj;
{
    char confirm[QBUFSZ], the_wand[BUFSZ];

    Strcpy(the_wand, yname(obj));
    Sprintf(confirm, "你确定你真的要折断%s?",
	safe_qbuf("", sizeof("你确定你真的要折断 ?"),
				the_wand, ysimple_name(obj), "魔杖"));
    if (yn(confirm) == 'n' ) return 0;

    if (nohands(youmonst.data)) {
	You_cant("没有手怎么折断%s？", the_wand);
	return 0;
    } else if (ACURR(A_STR) < 10) {
	You("的力气不足以折断%s!", the_wand);
	return 0;
    }
    pline("你高高的把%s举过你的%s后，把它用力的一分两断！",
	  the_wand, body_part(HEAD));
    return wand_explode(obj, TRUE);
}

/* This function takes care of the effects wands exploding, via
 * user-specified 'applying' as well as wands exploding by accident
 * during use (called by backfire() in zap.c)
 *
 * If the effect is directly recognisable as pertaining to a 
 * specific wand, the wand should be makeknown()
 * Otherwise, if there is an ambiguous or indirect but visible effect
 * the wand should be allowed to be named by the user.
 *
 * If there is no obvious effect,  do nothing. (Should this be changed
 * to letting the user call that type of wand?)
 *
 * hero_broke is nonzero if the user initiated the action that caused
 * the wand to explode (zapping or applying).
 */
int
wand_explode(obj, hero_broke)
    struct obj *obj;
    boolean hero_broke;
{
    static const char nothing_else_happens[] = "但是什么也没有发生...";
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    int expltype = EXPL_MAGICAL;
    char buf[BUFSZ];

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (carried(obj) ? obj->unpaid :
	    !obj->no_charge && costly_spot(obj->ox, obj->oy)) {
	if (hero_broke)
	check_unpaid(obj);		/* Extra charge for use */
	bill_dummy_object(obj);
    }

    current_wand = obj;		/* destroy_item might reset this */
    freeinv(obj);		/* hide it from destroy_item instead... */
    setnotworn(obj);		/* so we need to do this ourselves */

    if (obj->spe <= 0) {
	pline(nothing_else_happens);
	goto discard_broken_wand;
    }
    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
	pline(nothing_else_happens);
	goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
	dmg *= 4;
	goto wanexpl;
    case WAN_COLD:
	expltype = EXPL_FROSTY;
	dmg *= 2;
    case WAN_MAGIC_MISSILE:
    wanexpl:
	explode(u.ux, u.uy, ZT_MAGIC_MISSILE, dmg, WAND_CLASS, expltype);
	makeknown(obj->otyp);	/* explode described the effect */
	goto discard_broken_wand;
/*WAC for wands of fireball- no double damage
 * As well, effect is the same as fire, so no makeknown
 */
    case WAN_FIRE:
	dmg *= 2;
    case WAN_FIREBALL:
	expltype = EXPL_FIERY;
        explode(u.ux, u.uy, ZT_FIRE, dmg, WAND_CLASS, expltype);
	if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		!objects[obj->otyp].oc_uname)
        docall(obj);
	goto discard_broken_wand;
    case WAN_STRIKING:
	/* we want this before the explosion instead of at the very end */
	pline("一道冲击波自你手中的魔杖四散而出!");
	dmg = d(1 + obj->spe,6);	/* normally 2d12 */
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_UNDEAD_TURNING:
    case WAN_DRAINING:	/* KMH */
	affects_objects = TRUE;
	break;
    case WAN_TELEPORTATION:
		/* WAC make tele trap if you broke a wand of teleport */
		/* But make sure the spot is valid! */
	    if ((obj->spe > 2) && rn2(obj->spe - 2) && !level.flags.noteleport &&
		    !u.uswallow && !On_stairs(u.ux, u.uy) && (!IS_FURNITURE(levl[u.ux][u.uy].typ) &&
		    !IS_ROCK(levl[u.ux][u.uy].typ) &&
		    !closed_door(u.ux, u.uy) && !t_at(u.ux, u.uy))) {

			struct trap *ttmp;

			ttmp = maketrap(u.ux, u.uy, TELEP_TRAP);
			if (ttmp) {
				ttmp->madeby_u = 1;
				newsym(u.ux, u.uy); /* if our hero happens to be invisible */
				if (*in_rooms(u.ux,u.uy,SHOPBASE)) {
					/* shopkeeper will remove it */
					add_damage(u.ux, u.uy, 0L);             
				}
			}
		}
	affects_objects = TRUE;
	break;
    case WAN_CREATE_HORDE: /* More damage than Create monster */
	        dmg *= 2;
	        break;
    case WAN_HEALING:
    case WAN_EXTRA_HEALING:
		dmg = 0;
		break;
    default:
	break;
    }

    /* magical explosion and its visual effect occur before specific effects */
    explode(obj->ox, obj->oy, ZT_MAGIC_MISSILE, dmg ? rnd(dmg) : 0, WAND_CLASS,
	    EXPL_MAGICAL);

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
	bhitpos.x = x = obj->ox + xdir[i];
	bhitpos.y = y = obj->oy + ydir[i];
	if (!isok(x,y)) continue;

	if (obj->otyp == WAN_DIGGING) {
	    if(dig_check(BY_OBJECT, FALSE, x, y)) {
		if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
		    /* normally, pits and holes don't anger guards, but they
		     * do if it's a wall or door that's being dug */
		    watch_dig((struct monst *)0, x, y, TRUE);
		    if (*in_rooms(x,y,SHOPBASE)) shop_damage = TRUE;
		}		    
		digactualhole(x, y, BY_OBJECT,
			      (rn2(obj->spe) < 3 || !Can_dig_down(&u.uz)) ?
			       PIT : HOLE);
	    }
	    continue;
/* WAC catch Create Horde wands too */
/* MAR make the monsters around you */
	} else if(obj->otyp == WAN_CREATE_MONSTER
                || obj->otyp == WAN_CREATE_HORDE) {
	    /* u.ux,u.uy creates it near you--x,y might create it in rock */
	    (void) makemon((struct permonst *)0, u.ux, u.uy, NO_MM_FLAGS);
	    continue;
	} else {
	    if (x == u.ux && y == u.uy) {
		/* teleport objects first to avoid race with tele control and
		   autopickup.  Other wand/object effects handled after
		   possible wand damage is assessed */
		if (obj->otyp == WAN_TELEPORTATION &&
		    affects_objects && level.objects[x][y]) {
		    (void) bhitpile(obj, bhito, x, y);
		    if (flags.botl) bot();		/* potion effects */
			/* makeknown is handled in zapyourself */
		}
		damage = zapyourself(obj, FALSE);
		if (damage) {
		    if (hero_broke) {
		    Sprintf(buf, "因为%s折断魔杖的行为而死", uhim());
		    losehp(damage, buf, NO_KILLER_PREFIX);
		    } else
			losehp(damage, "爆炸的魔杖", KILLED_BY_AN);
		}
		if (flags.botl) bot();		/* blindness */
	    } else if ((mon = m_at(x, y)) != 0 && !DEADMONSTER(mon)) {
		(void) bhitm(mon, obj);
	     /* if (flags.botl) bot(); */
	    }
	    if (affects_objects && level.objects[x][y]) {
		(void) bhitpile(obj, bhito, x, y);
		if (flags.botl) bot();		/* potion effects */
	    }
	}
    }

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage) pay_for_damage("在地板上挖坑", FALSE);

    if (obj->otyp == WAN_LIGHT)
	litroom(TRUE, obj);	/* only needs to be done once */

 discard_broken_wand:
    obj = current_wand;		/* [see dozap() and destroy_item()] */
    current_wand = 0;
    if (obj)
	delobj(obj);
    nomul(0);
    return 1;
}

STATIC_OVL boolean
uhave_graystone()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(is_graystone(otmp))
			return TRUE;
	return FALSE;
}

STATIC_OVL void
add_class(cl, class)
char *cl;
char class;
{
	char tmp[2];
	tmp[0] = class;
	tmp[1] = '\0';
	Strcat(cl, tmp);
}

int
doapply()
{
	struct obj *obj;
	register int res = 1;
	register boolean can_use = FALSE;
	char class_list[MAXOCLASSES+2];

	if(check_capacity((char *)0)) return (0);

	if (carrying(POT_OIL) || uhave_graystone())
		Strcpy(class_list, tools_too);
	else
		Strcpy(class_list, tools);
	if (carrying(CREAM_PIE) || carrying(EUCALYPTUS_LEAF))
		add_class(class_list, FOOD_CLASS);

	obj = getobj(class_list, "使用");
	if(!obj) return 0;

	if (obj->oartifact && !touch_artifact(obj, &youmonst))
	    return 1;	/* evading your grasp costs a turn; just be
			   grateful that you don't drop it as well */

	if (obj->oclass == WAND_CLASS)
	    return do_break_wand(obj);

	switch(obj->otyp){
	case BLINDFOLD:
	case LENSES:
		if (obj == ublindf) {
		    if (!cursed(obj)) Blindf_off(obj);
		} else if (!ublindf)
		    Blindf_on(obj);
		else You("已经%s.",
			ublindf->otyp == TOWEL ?     "拿毛巾蒙住脸了" :
			ublindf->otyp == BLINDFOLD ? "戴着一副眼罩了" :
						     "戴着眼镜了");
		break;
	case CREAM_PIE:
		res = use_cream_pie(obj);
		break;
	case BULLWHIP:
		res = use_whip(obj);
		break;
	case GRAPPLING_HOOK:
		res = use_grapple(obj);
		break;
	case LARGE_BOX:
	case CHEST:
	case ICE_BOX:
	case SACK:
	case BAG_OF_HOLDING:
	case OILSKIN_SACK:
		res = use_container(&obj, 1);
		break;
	case BAG_OF_TRICKS:
		bagotricks(obj);
		break;
	case CAN_OF_GREASE:
		use_grease(obj);
		break;
#ifdef TOURIST
	case CREDIT_CARD:
#endif
	case LOCK_PICK:
	case SKELETON_KEY:
		(void) pick_lock(&obj);
		break;
	case PICK_AXE:
	case DWARVISH_MATTOCK: /* KMH, balance patch -- the mattock is a pick, too */
		res = use_pick_axe(obj);
		break;
	case FISHING_POLE:
		res = use_pole(obj);
		break;
	case TINNING_KIT:
		use_tinning_kit(obj);
		break;
	case LEASH:
		use_leash(obj);
		break;
#ifdef STEED
	case SADDLE:
		res = use_saddle(obj);
		break;
#endif
	case MAGIC_WHISTLE:
		use_magic_whistle(obj);
		break;
	case TIN_WHISTLE:
		use_whistle(obj);
		break;
	case EUCALYPTUS_LEAF:
		/* MRKR: Every Australian knows that a gum leaf makes an */
		/*	 excellent whistle, especially if your pet is a  */
		/*	 tame kangaroo named Skippy.			 */
		if (obj->blessed) {
		    use_magic_whistle(obj);
		    /* sometimes the blessing will be worn off */
		    if (!rn2(49)) {
			if (!Blind) {
			    char buf[BUFSZ];

			    pline("%s %s %s.", Shk_Your(buf, obj),
				  aobjnam(obj, "发出了"), hcolor("棕色的光芒"));
			    obj->bknown = 1;
			}
			unbless(obj);
		    }
		} else {
		    use_whistle(obj);
		}
		break;
	case STETHOSCOPE:
		res = use_stethoscope(obj);
		break;
	case MIRROR:
		res = use_mirror(obj);
		break;
# ifdef P_SPOON
	case SPOON:
		pline("这个奇特的勺子做工非常精良，你想用它做什么?");
		break;
# endif /* P_SPOON */
	case BELL:
	case BELL_OF_OPENING:
		use_bell(&obj);
		break;
	case CANDELABRUM_OF_INVOCATION:
		use_candelabrum(obj);
		break;
	case WAX_CANDLE:
/* STEPHEN WHITE'S NEW CODE */           
	case MAGIC_CANDLE:
	case TALLOW_CANDLE:
		use_candle(&obj);
		break;
#ifdef LIGHTSABERS
	case GREEN_LIGHTSABER:
#ifdef D_SABER
  	case BLUE_LIGHTSABER:
#endif
	case RED_LIGHTSABER:
	case RED_DOUBLE_LIGHTSABER:
		if (uwep != obj && !wield_tool(obj, (const char *)0)) break;
		/* Fall through - activate via use_lamp */
#endif
	case OIL_LAMP:
	case MAGIC_LAMP:
	case BRASS_LANTERN:
		use_lamp(obj);
		break;
	case TORCH:
	        res = use_torch(obj);
		break;
	case POT_OIL:
		light_cocktail(obj);
		break;
#ifdef TOURIST
	case EXPENSIVE_CAMERA:
		res = use_camera(obj);
		break;
#endif
	case TOWEL:
		res = use_towel(obj);
		break;
	case CRYSTAL_BALL:
		use_crystal_ball(obj);
		break;
/* STEPHEN WHITE'S NEW CODE */
/* KMH, balance patch -- source of abuse */
#if 0
	case ORB_OF_ENCHANTMENT:
	    if(obj->spe > 0) {
		
		check_unpaid(obj);
		if(uwep && (uwep->oclass == WEAPON_CLASS ||
			    uwep->otyp == PICK_AXE ||
			    uwep->otyp == UNICORN_HORN)) {
		if (uwep->spe < 5) {
		if (obj->blessed) {
				if (!Blind) pline("你的%s发出了银色的光.",xname(uwep));
				uwep->spe += rnd(2);
		} else if (obj->cursed) {                               
				if (!Blind) pline("你的%s发出了黑色的光.",xname(uwep));
				uwep->spe -= rnd(2);
		} else {
				if (rn2(3)) {
					if (!Blind) pline("你的%s发出了一小会的银色光芒." ,xname(uwep));
					uwep->spe += 1;
				} else {
					if (!Blind) pline("你的%s发出了一阵的黑色光芒." ,xname(uwep));
					uwep->spe -= 1;
				}
		}
		} else pline("似乎没什么发生.");
		
		if (uwep->spe > 5) uwep->spe = 5;
				
		} else pline("水晶球发出了一小会的光芒.");
		consume_obj_charge(obj, FALSE);
	    
	    } else pline("水晶球没能量了.");
	    break;
	case ORB_OF_CHARGING:
		if(obj->spe > 0) {
			register struct obj *otmp;
			makeknown(ORB_OF_CHARGING);
			consume_obj_charge(obj, TRUE);
			otmp = getobj(all_count, "充能");
			if (!otmp) break;
			recharge(otmp, obj->cursed ? -1 : (obj->blessed ? 1 : 0));
		} else pline("这个水晶球已经没能量了.");
		break;
	case ORB_OF_DESTRUCTION:
		useup(obj);
		pline("在你试图使用它的时候它突然爆炸了!");
		explode(u.ux, u.uy, ZT_SPELL(ZT_MAGIC_MISSILE), d(12,6), WAND_CLASS);
		check_unpaid(obj);
		break;
#endif
	case MAGIC_MARKER:
		res = dowrite(obj);
		break;
	case TIN_OPENER:
		if(!carrying(TIN)) {
			You("没有要开的罐头.");
			goto xit;
		}
		You("不能在不吃或者不丢掉里头的食物的情况下开罐头，这样不环保.");
		if(flags.verbose)
			pline("准备吃东西的话，按e命令即可.");
		if(obj != uwep)
    pline("你得拿着开罐器才会让开罐变得更简单.");
		goto xit;

	case FIGURINE:
		use_figurine(&obj);
		break;
	case UNICORN_HORN:
		use_unicorn_horn(obj);
		break;
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
	case WOODEN_HARP:
	case MAGIC_HARP:
	case BUGLE:
	case LEATHER_DRUM:
	case DRUM_OF_EARTHQUAKE:
	/* KMH, balance patch -- removed
	case PAN_PIPE_OF_SUMMONING:                
	case PAN_PIPE_OF_THE_SEWERS:
	case PAN_PIPE:*/
		res = do_play_instrument(obj);
		break;
	case MEDICAL_KIT:        
		if (Role_if(PM_HEALER)) can_use = TRUE;
		else if ((Role_if(PM_PRIEST) || Role_if(PM_MONK) ||
			Role_if(PM_UNDEAD_SLAYER) || Role_if(PM_SAMURAI)) &&
			!rn2(2)) can_use = TRUE;
		else if(!rn2(4)) can_use = TRUE;

		if (obj->cursed && rn2(3)) can_use = FALSE;
		if (obj->blessed && rn2(3)) can_use = TRUE;  

		makeknown(MEDICAL_KIT);
		if (obj->cobj) {
		    struct obj *otmp;
		    for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
			if (otmp->otyp == PILL)
			    break;
		    if (!otmp)
			You_cant("在%s里头找到药丸了.", yname(obj));
		    else if (!is_edible(otmp))
			You("虽然找到但是没法吃%s.",
			  yname(obj));
		    else {
			check_unpaid(obj);
			if (otmp->quan > 1L) {
			    otmp->quan--;
			    obj->owt = weight(obj);
			} else {
			    obj_extract_self(otmp);
			    obfree(otmp, (struct obj *)0);
			}
			/*
			 * Note that while white and pink pills share the
			 * same otyp value, they are quite different.
			 */
			You("从%s里头拿出一个药丸然后吃了下去.",
				yname(obj));
			if (can_use) {
			    if (Sick) make_sick(0L, (char *) 0,TRUE ,SICK_ALL);
			    else if (Blinded > (long)(u.ucreamed+1))
				make_blinded(u.ucreamed ?
					(long)(u.ucreamed+1) : 0L, TRUE);
			    else if (HHallucination)
				make_hallucinated(0L, TRUE, 0L);
			    else if (Vomiting) make_vomiting(0L, TRUE);
			    else if (HConfusion) make_confused(0L, TRUE);
			    else if (HStun) make_stunned(0L, TRUE);
			    else if (u.uhp < u.uhpmax) {
				u.uhp += rn1(10,10);
				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
				You_feel("好多了.");
				flags.botl = TRUE;
			    } else pline(nothing_happens);
			} else if (!rn2(3))
			    pline("没什么发生.");
			else if (!Sick)
			    make_sick(rn1(10,10), "坏药丸！", TRUE,
			      SICK_VOMITABLE);
			else {
			    You("看起来感觉更糟糕了!");
			    losehp(rn1(10,10), "胡乱吃药", KILLED_BY);
			}
		    }
		} else You("看起来你的剩下的药都用完了。");
		break;
	case HORN_OF_PLENTY:	/* not a musical instrument */
		if (obj->spe > 0) {
		    struct obj *otmp;
		    const char *what;

		    consume_obj_charge(obj, TRUE);
		    if (!rn2(13)) {
			otmp = mkobj(POTION_CLASS, FALSE);
			/* KMH, balance patch -- rewritten */
			while ((otmp->otyp == POT_SICKNESS) ||
					objects[otmp->otyp].oc_magic)
			    otmp->otyp = rnd_class(POT_BOOZE, POT_WATER);
			what = "一瓶药水";
		    } else {
			otmp = mkobj(FOOD_CLASS, FALSE);
			if (otmp->otyp == FOOD_RATION && !rn2(7))
			    otmp->otyp = LUMP_OF_ROYAL_JELLY;
			what = "一些吃的";
		    }
		    pline("%s从号角里头涌了出来.", what);
		    otmp->blessed = obj->blessed;
		    otmp->cursed = obj->cursed;
		    otmp->owt = weight(otmp);
		    otmp = hold_another_object(otmp, u.uswallow ?
				       "啊！你没抓稳%s!" :
					(Is_airlevel(&u.uz) ||
					 Is_waterlevel(&u.uz) ||
					 levl[u.ux][u.uy].typ < IRONBARS ||
					 levl[u.ux][u.uy].typ >= ICE) ?
					       "哎呀!%s从你身边滑下去了！" :
					       "不好！%s掉下去了!",
					       The(aobjnam(otmp, "滑到")),
					       (const char *)0);
		    makeknown(HORN_OF_PLENTY);
		} else
		    pline(nothing_happens);
		break;
	case LAND_MINE:
	case BEARTRAP:
		use_trap(obj);
		break;
	case FLINT:
	case LUCKSTONE:
	case LOADSTONE:
	case TOUCHSTONE:
	case HEALTHSTONE:
	case WHETSTONE:
		use_stone(obj);
		break;
#ifdef FIREARMS
	case ASSAULT_RIFLE:
		/* Switch between WP_MODE_SINGLE, WP_MODE_BURST and WP_MODE_AUTO */

		if (obj->altmode == WP_MODE_AUTO) {
			obj->altmode = WP_MODE_BURST;
		} else if (obj->altmode == WP_MODE_BURST) {
			obj->altmode = WP_MODE_SINGLE;
		} else {
			obj->altmode = WP_MODE_AUTO;
		}
		
		You("把%s切换到%s模式.", yname(obj),
			((obj->altmode == WP_MODE_SINGLE) ? "点射" :
			 ((obj->altmode == WP_MODE_BURST) ? "连发" :
			  "全自动")));
		break;	
	case AUTO_SHOTGUN:
	case SUBMACHINE_GUN:		
		if (obj->altmode == WP_MODE_AUTO) obj-> altmode = WP_MODE_SINGLE;
		else obj->altmode = WP_MODE_AUTO;
		You("把%s切换到%s模式.", yname(obj),
			(obj->altmode ? "半自动" : "全自动"));
		break;
	case FRAG_GRENADE:
	case GAS_GRENADE:
		if (!obj->oarmed) {
			You("把%s的插销拔掉.", yname(obj));
			arm_bomb(obj, TRUE);
		} else pline("的插销已经被拔掉了!");
		break;
	case STICK_OF_DYNAMITE:
		light_cocktail(obj);
		break;
#endif
	default:
		/* KMH, balance patch -- polearms can strike at a distance */
		if (is_pole(obj)) {
			res = use_pole(obj);
			break;
		} else if (is_pick(obj) || is_axe(obj)) {
			res = use_pick_axe(obj);
			break;
		}
		pline("这玩意不能这么用.");
	xit:
		nomul(0);
		return 0;
	}
	if (res && obj && obj->oartifact) arti_speak(obj);
	nomul(0);
	return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(is_horn)
	boolean is_horn;
{
	int unfixable_trbl = 0;

	if (Stoned) unfixable_trbl++;
	if (Strangled) unfixable_trbl++;
	if (Wounded_legs
#ifdef STEED
		    && !u.usteed
#endif
				) unfixable_trbl++;
	if (Slimed) unfixable_trbl++;
	/* lycanthropy is not desirable, but it doesn't actually make you feel
	   bad */

	/* we'll assume that intrinsic stunning from being a bat/stalker
	   doesn't make you feel bad */
	if (!is_horn) {
	    if (Confusion) unfixable_trbl++;
	    if (Sick) unfixable_trbl++;
	    if (HHallucination) unfixable_trbl++;
	    if (Vomiting) unfixable_trbl++;
	    if (HStun) unfixable_trbl++;
	}
	return unfixable_trbl;
}

#endif /* OVLB */

/*apply.c*/
