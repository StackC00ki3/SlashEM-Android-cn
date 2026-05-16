/*	SCCS Id: @(#)dig.c	3.4	2003/03/23	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
/* #define DEBUG */	/* turn on for diagnostics */

#ifdef OVLB

static NEARDATA boolean did_dig_msg;

STATIC_DCL boolean NDECL(rm_waslit);
STATIC_DCL void FDECL(mkcavepos, (XCHAR_P,XCHAR_P,int,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void FDECL(mkcavearea, (BOOLEAN_P));
STATIC_DCL int FDECL(dig_typ, (struct obj *,XCHAR_P,XCHAR_P));
STATIC_DCL int NDECL(dig);
STATIC_DCL schar FDECL(fillholetyp, (int, int));
STATIC_DCL void NDECL(dig_up_grave);

/* Indices returned by dig_typ() */
#define DIGTYP_UNDIGGABLE 0
#define DIGTYP_ROCK       1
#define DIGTYP_STATUE     2
#define DIGTYP_BOULDER    3
#define DIGTYP_DOOR       4
#define DIGTYP_TREE       5


STATIC_OVL boolean
rm_waslit()
{
    register xchar x, y;

    if(levl[u.ux][u.uy].typ == ROOM && levl[u.ux][u.uy].waslit)
	return(TRUE);
    for(x = u.ux-2; x < u.ux+3; x++)
	for(y = u.uy-1; y < u.uy+2; y++)
	    if(isok(x,y) && levl[x][y].waslit) return(TRUE);
    return(FALSE);
}

/* Change level topology.  Messes with vision tables and ignores things like
 * boulders in the name of a nice effect.  Vision will get fixed up again
 * immediately after the effect is complete.
 */
STATIC_OVL void
mkcavepos(x, y, dist, waslit, rockit)
    xchar x,y;
    int dist;
    boolean waslit, rockit;
{
    register struct rm *lev;

    if(!isok(x,y)) return;
    lev = &levl[x][y];

    if(rockit) {
	register struct monst *mtmp;

	if(IS_ROCK(lev->typ)) return;
	if(t_at(x, y)) return; /* don't cover the portal */
	if ((mtmp = m_at(x, y)) != 0)	/* make sure crucial monsters survive */
	    if(!passes_walls(mtmp->data)) (void) rloc(mtmp, FALSE);
    } else if(lev->typ == ROOM) return;

    unblock_point(x,y);	/* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if(dist < 3) lev->lit = (rockit ? FALSE : TRUE);
    if(waslit) lev->waslit = (rockit ? FALSE : TRUE);
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 3 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;
    lev->typ = (rockit ? STONE : ROOM);
    if(dist >= 3)
	impossible("石头不存在？", dist);
    if(Blind)
	feel_location(x, y);
    else newsym(x,y);
}

STATIC_OVL void
mkcavearea(rockit)
register boolean rockit;
{
    int dist;
    xchar xmin = u.ux, xmax = u.ux;
    xchar ymin = u.uy, ymax = u.uy;
    register xchar i;
    register boolean waslit = rm_waslit();

    if(rockit) pline("咣当!你周围的石头塌下来了!");
    else pline("一股神秘的力量在你周围%s一个空洞!",
	     (levl[u.ux][u.uy].typ == CORR) ? "创造出了" : "搞出了");
    display_nhwindow(WIN_MESSAGE, TRUE);

    for(dist = 1; dist <= 2; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist < 2) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkcavepos(i, ymin, dist, waslit, rockit);
		mkcavepos(i, ymax, dist, waslit, rockit);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkcavepos(xmin, i, dist, waslit, rockit);
	    mkcavepos(xmax, i, dist, waslit, rockit);
	}

	flush_screen(1);	/* make sure the new glyphs shows up */
	delay_output();
    }

    if(!rockit && levl[u.ux][u.uy].typ == CORR) {
	levl[u.ux][u.uy].typ = ROOM;
	if(waslit) levl[u.ux][u.uy].waslit = TRUE;
	newsym(u.ux, u.uy); /* in case player is invisible */
    }

    vision_full_recalc = 1;	/* everything changed */
}

/* When digging into location <x,y>, what are you actually digging into? */
STATIC_OVL int
dig_typ(otmp, x, y)
struct obj *otmp;
xchar x, y;
{
	boolean ispick = is_pick(otmp);

	return (ispick && sobj_at(STATUE, x, y) ? DIGTYP_STATUE :
		ispick && sobj_at(BOULDER, x, y) ? DIGTYP_BOULDER :
		closed_door(x, y) ? DIGTYP_DOOR :
		IS_TREE(levl[x][y].typ) ?
			(ispick ? DIGTYP_UNDIGGABLE : DIGTYP_TREE) :
		ispick && IS_ROCK(levl[x][y].typ) &&
			(!level.flags.arboreal || IS_WALL(levl[x][y].typ)) ?
			DIGTYP_ROCK : DIGTYP_UNDIGGABLE);
}

boolean
is_digging()
{
	if (occupation == dig) {
	    return TRUE;
	}
	return FALSE;
}

#define BY_YOU		(&youmonst)
#define BY_OBJECT	((struct monst *)0)

boolean
dig_check(madeby, verbose, x, y)
	struct monst	*madeby;
	boolean		verbose;
	int		x, y;
{
	struct trap *ttmp = t_at(x, y);
	const char *verb =
	    (madeby != BY_YOU || !uwep || is_pick(uwep)) ? "挖掘" :
#ifdef LIGHTSABERS
	    is_lightsaber(uwep) ? "切断" :
#endif
	    "砍断";

	if (On_stairs(x, y)) {
	    if (x == xdnladder || x == xupladder) {
		if(verbose) pline_The("楼梯丝毫不动.");
	    } else if(verbose) pline_The("楼梯太难%s.", verb);
	    return(FALSE);
	/* ALI - Artifact doors */
	} else if (IS_DOOR(levl[x][y].typ) && artifact_door(x, y)) {
	    if(verbose) pline_The("%s根本就挖不动.",
				  surface(x,y));
	    return(FALSE);
	} else if (IS_THRONE(levl[x][y].typ) && madeby != BY_OBJECT) {
	    if(verbose) pline_The("你很难把王座打碎.");
	    return(FALSE);
	} else if (IS_ALTAR(levl[x][y].typ) && (madeby != BY_OBJECT ||
				Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
	    if(verbose) pline_The("祭坛根本就挖不动.");
	    return(FALSE);
	} else if (Is_airlevel(&u.uz)) {
	    if(verbose) You("没法%s空气.", verb);
	    return(FALSE);
	} else if (Is_waterlevel(&u.uz)) {
	    if(verbose) pline_The("周围的水搅动了一下后就恢复原来的样子了.");
	    return(FALSE);
	} else if ((IS_ROCK(levl[x][y].typ) && levl[x][y].typ != SDOOR &&
		      (levl[x][y].wall_info & W_NONDIGGABLE) != 0)
		|| (ttmp &&
		      (ttmp->ttyp == MAGIC_PORTAL || !Can_dig_down(&u.uz)))) {
	    if(verbose) pline_The("这边的%s太难%s了.",
				  surface(x,y), verb);
	    return(FALSE);
	} else if (sobj_at(BOULDER, x, y)) {
	    if(verbose) There("没有足够的空间让你%s.", verb);
	    return(FALSE);
	} else if (madeby == BY_OBJECT &&
		    /* the block against existing traps is mainly to
		       prevent broken wands from turning holes into pits */
		    (ttmp || is_pool(x,y) || is_lava(x,y))) {
	    /* digging by player handles pools separately */
	    return FALSE;
	}
	return(TRUE);
}

STATIC_OVL int
dig()
{
	register struct rm *lev;
	register xchar dpx = digging.pos.x, dpy = digging.pos.y;
	register boolean ispick = uwep && is_pick(uwep);
	const char *verb =
	    (!uwep || is_pick(uwep)) ? "挖掘" :
#ifdef LIGHTSABERS
	    is_lightsaber(uwep) ? "切穿" :
#endif
	    "砍断";
	int bonus;

	lev = &levl[dpx][dpy];
	/* perhaps a nymph stole your pick-axe while you were busy digging */
	/* or perhaps you teleported away */
	/* WAC allow lightsabers */
	if (u.uswallow || !uwep || (!ispick &&
#ifdef LIGHTSABERS
		(!is_lightsaber(uwep) || !uwep->lamplit) &&
#endif
		!is_axe(uwep)) ||
	    !on_level(&digging.level, &u.uz) ||
	    ((digging.down ? (dpx != u.ux || dpy != u.uy)
			   : (distu(dpx,dpy) > 2))))
		return(0);

	if (digging.down) {
	    if(!dig_check(BY_YOU, TRUE, u.ux, u.uy)) return(0);
	} else { /* !digging.down */
	    if (IS_TREE(lev->typ) && !may_dig(dpx,dpy) &&
			dig_typ(uwep, dpx, dpy) == DIGTYP_TREE) {
		pline("这个树好像是个雕像.");
		return(0);
	    }
	    /* ALI - Artifact doors */
	    if (IS_ROCK(lev->typ) && !may_dig(dpx,dpy) &&
	    		dig_typ(uwep, dpx, dpy) == DIGTYP_ROCK ||
		    IS_DOOR(lev->typ) && artifact_door(dpx, dpy)) {
		pline("这个%s实在是太难%s了.",
			IS_DOOR(lev->typ) ? "门" : "墙", verb);
		return(0);
	    }
	}
	if(Fumbling &&
#ifdef LIGHTSABERS
		/* Can't exactly miss holding a lightsaber to the wall */
		!is_lightsaber(uwep) &&
#endif
		!rn2(3)) {
	    switch(rn2(3)) {
	    case 0:
		if(!welded(uwep)) {
		    You("手抖了一下后一不小心把你的%s丢掉了.", xname(uwep));
		    dropx(uwep);
		} else {
#ifdef STEED
		    if (u.usteed)
			Your("%s %s的同时%s %s!",
			     xname(uwep),
			     otense(uwep, "被反弹回来"), otense(uwep, "打中了"),
			     mon_nam(u.usteed));
		    else
#endif
			pline("啊!%s %s然后%s!",
			      xname(uwep),
			      otense(uwep, "被弹回来"), otense(uwep, "打中了你"));
		    set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		}
		break;
	    case 1:
		pline("砰!你用%s的宽面狠狠的击打!",
		      the(xname(uwep)));
		break;
	    default: Your("胡乱挥舞没打中.");
		break;
	    }
	    return(0);
	}

	bonus = 10 + rn2(5) + abon() + uwep->spe - greatest_erosion(uwep) + u.udaminc;
	if (Race_if(PM_DWARF))
	    bonus *= 2;
#ifdef LIGHTSABERS
	if (is_lightsaber(uwep))
	    bonus -= rn2(20); /* Melting a hole takes longer */
#endif

	digging.effort += bonus;

	if (digging.down) {
		register struct trap *ttmp;

		if (digging.effort > 250) {
		    (void) dighole(FALSE);
		    (void) memset((genericptr_t)&digging, 0, sizeof digging);
		    return(0);	/* done with digging */
		}

		if (digging.effort <= 50 ||
#ifdef LIGHTSABERS
		    is_lightsaber(uwep) ||
#endif
		    ((ttmp = t_at(dpx,dpy)) != 0 &&
			(ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT ||
			 ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)))
		    return(1);

		if (IS_ALTAR(lev->typ)) {
		    altar_wrath(dpx, dpy);
		    angry_priest();
		}

		if (dighole(TRUE)) {	/* make pit at <u.ux,u.uy> */
		    digging.level.dnum = 0;
		    digging.level.dlevel = -1;
		}
		return(0);
	}

	if (digging.effort > 100) {
		register const char *digtxt, *dmgtxt = (const char*) 0;
		register struct obj *obj;
		register boolean shopedge = *in_rooms(dpx, dpy, SHOPBASE);

		if ((obj = sobj_at(STATUE, dpx, dpy)) != 0) {
			if (break_statue(obj))
				digtxt = "雕像碎掉了.";
			else
				/* it was a statue trap; break_statue()
				 * printed a message and updated the screen
				 */
				digtxt = (char *)0;
		} else if ((obj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			struct obj *bobj;

			fracture_rock(obj);
			if ((bobj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
			digtxt = "巨石碎掉了.";
		} else if (lev->typ == STONE || lev->typ == SCORR ||
				IS_TREE(lev->typ)) {
			if(Is_earthlevel(&u.uz)) {
			    if(uwep->blessed && !rn2(3)) {
				mkcavearea(FALSE);
				goto cleanup;
			    } else if((uwep->cursed && !rn2(4)) ||
					  (!uwep->blessed && !rn2(6))) {
				mkcavearea(TRUE);
				goto cleanup;
			    }
			}
			if (IS_TREE(lev->typ)) {
			    digtxt = "你把树砍掉了.";
			    lev->typ = ROOM;
			    if (!rn2(5)) (void) rnd_treefruit_at(dpx, dpy);
			} else {
			    digtxt = "你成功的砍掉了一点石头.";
			    lev->typ = CORR;
			}
		} else if(IS_WALL(lev->typ)) {
			if(shopedge) {
			    add_damage(dpx, dpy, 10L * ACURRSTR);
			    dmgtxt = "损害";
			}
			if (level.flags.is_maze_lev) {
			    lev->typ = ROOM;
			} else if (level.flags.is_cavernous_lev &&
				   !in_town(dpx, dpy)) {
			    lev->typ = CORR;
			} else {
			    lev->typ = DOOR;
			    lev->doormask = D_NODOOR;
			}
			digtxt = "你在墙上开了个口子.";
		} else if(lev->typ == SDOOR) {
			cvt_sdoor_to_door(lev);	/* ->typ = DOOR */
			digtxt = "你挖出了一个暗门!";
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else if(closed_door(dpx, dpy)) {
			digtxt = "你直接把墙挖开了.";
			if(shopedge) {
			    add_damage(dpx, dpy, 400L);
			    dmgtxt = "破坏";
			}
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else return(0); /* statue or boulder got taken */

		if(!does_block(dpx,dpy,&levl[dpx][dpy]))
		    unblock_point(dpx,dpy);	/* vision:  can see through */
		if(Blind)
		    feel_location(dpx, dpy);
		else
		    newsym(dpx, dpy);
		if(digtxt && !digging.quiet) pline(digtxt); /* after newsym */
		if(dmgtxt)
		    pay_for_damage(dmgtxt, FALSE);

		if(Is_earthlevel(&u.uz) && !rn2(3)) {
		    register struct monst *mtmp;

		    switch(rn2(2)) {
		      case 0:
			mtmp = makemon(&mons[PM_EARTH_ELEMENTAL],
					dpx, dpy, NO_MM_FLAGS);
			break;
		      default:
			mtmp = makemon(&mons[PM_XORN],
					dpx, dpy, NO_MM_FLAGS);
			break;
		    }
		    if(mtmp) pline_The("碎片重新聚合起来后活了过来!");
		}
		if(IS_DOOR(lev->typ) && (lev->doormask & D_TRAPPED)) {
			lev->doormask = D_NODOOR;
			b_trapped("门", 0);
			newsym(dpx, dpy);
		}
cleanup:
		digging.lastdigtime = moves;
		digging.quiet = FALSE;
		digging.level.dnum = 0;
		digging.level.dlevel = -1;
		return(0);
	} else {		/* not enough effort has been spent yet */
		static const char *const d_target[6] = {
			"", "石头", "雕像", "巨石", "门", "树"
		};
		int dig_target = dig_typ(uwep, dpx, dpy);

		if (IS_WALL(lev->typ) || dig_target == DIGTYP_DOOR) {
		    if(*in_rooms(dpx, dpy, SHOPBASE)) {
			pline("这个%s看起来根本%s不动.",
			      IS_DOOR(lev->typ) ? "门" : "墙", verb);
			return(0);
		    }
		} else if (!IS_ROCK(lev->typ) && dig_target == DIGTYP_ROCK)
		    return(0); /* statue or boulder got taken */
		if(!did_dig_msg) {
#ifdef LIGHTSABERS
		    if (is_lightsaber(uwep)) You("直接用光剑烧穿了%s.",
			the(d_target[dig_target]));
		    else
#endif
		    You("用尽全身力量狠狠的挖掘%s.",
			d_target[dig_target]);
		    did_dig_msg = TRUE;
		}
	}
	return(1);
}

/* When will hole be finished? Very rough indication used by shopkeeper. */
int
holetime()
{
	if(occupation != dig || !*u.ushops) return(-1);
	return ((250 - digging.effort) / 20);
}

/* Return typ of liquid to fill a hole with, or ROOM, if no liquid nearby */
STATIC_OVL
schar
fillholetyp(x,y)
int x, y;
{
    register int x1, y1;
    int lo_x = max(1,x-1), hi_x = min(x+1,COLNO-1),
	lo_y = max(0,y-1), hi_y = min(y+1,ROWNO-1);
    int pool_cnt = 0, moat_cnt = 0, lava_cnt = 0;

    for (x1 = lo_x; x1 <= hi_x; x1++)
	for (y1 = lo_y; y1 <= hi_y; y1++)
	    if (levl[x1][y1].typ == POOL)
		pool_cnt++;
	    else if (levl[x1][y1].typ == MOAT ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_MOAT))
		moat_cnt++;
	    else if (levl[x1][y1].typ == LAVAPOOL ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_LAVA))
		lava_cnt++;
    pool_cnt /= 3;		/* not as much liquid as the others */

    if (lava_cnt > moat_cnt + pool_cnt && rn2(lava_cnt + 1))
	return LAVAPOOL;
    else if (moat_cnt > 0 && rn2(moat_cnt + 1))
	return MOAT;
    else if (pool_cnt > 0 && rn2(pool_cnt + 1))
	return POOL;
    else
	return ROOM;
}

void
digactualhole(x, y, madeby, ttyp)
register int	x, y;
struct monst	*madeby;
int ttyp;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);	/* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);
	boolean wont_fall = Levitation || Flying;

	if (u.utrap && u.utraptype == TT_INFLOOR) u.utrap = 0;

	/* these furniture checks were in dighole(), but wand
	   breaking bypasses that routine and calls us directly */
	if (IS_FOUNTAIN(lev->typ)) {
	    dogushforth(FALSE);
	    SET_FOUNTAIN_WARNED(x,y);		/* force dryup */
	    dryup(x, y, madeby_u);
	    return;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
	    breaksink(x, y);
	    return;
	} else if (IS_TOILET(lev->typ)) {
		breaktoilet(u.ux,u.uy);
#endif
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(x, y) >= 0)) {
	    int bx = x, by = y;
	    /* if under the portcullis, the bridge is adjacent */
	    (void) find_drawbridge(&bx, &by);
	    destroy_drawbridge(bx, by);
	    return;
	}

	if (ttyp != PIT && !Can_dig_down(&u.uz)) {
	    impossible("错误：无法挖掘墙体.",
		       defsyms[trap_to_defsym(ttyp)].explanation);
	    ttyp = PIT;
	}

	/* maketrap() might change it, also, in this situation,
	   surface() returns an inappropriate string for a grave */
	if (IS_GRAVE(lev->typ))
	    Strcpy(surface_type, "坟墓");
	else
	    Strcpy(surface_type, surface(x,y));
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[x][y];
	ttmp->tseen = (madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	if (ttyp == PIT) {

	    if(madeby_u) {
		You("在%s上挖了个坑.", surface_type);
		if (shopdoor) pay_for_damage("破坏地板", FALSE);
	    } else if (!madeby_obj && canseemon(madeby))
		pline("%s在%s上挖了个坑.", Monnam(madeby), surface_type);
	    else if (cansee(x, y) && flags.verbose)
		pline("一个%s出现在了地上.", surface_type);

	    if(at_u) {
		if (!wont_fall) {
		    if (!Passes_walls)
			u.utrap = rn1(4,2);
		    u.utraptype = TT_PIT;
		    vision_full_recalc = 1;	/* vision limits change */
		} else
		    u.utrap = 0;
		if (oldobjs != newobjs)	/* something unearthed */
			(void) pickup(1);	/* detects pit */
	    } else if(mtmp) {
		if(is_flyer(mtmp->data) || is_floater(mtmp->data)) {
		    if(canseemon(mtmp))
			pline("%s %s那个坑.", Monnam(mtmp),
						     (is_flyer(mtmp->data)) ?
						     "飞过了" : "飘过了");
		} else if(mtmp != madeby)
		    (void) mintrap(mtmp);
	    }
	} else {	/* was TRAPDOOR now a HOLE*/

	    if(madeby_u)
		You("在%s上面搞出了一个坑.", surface_type);
	    else if(!madeby_obj && canseemon(madeby))
		pline("%s在%s上面挖了个坑.",
		      Monnam(madeby), surface_type);
	    else if(cansee(x, y) && flags.verbose)
		pline("一个洞在%s上突然出现.", surface_type);

	    if (at_u) {
		if (!u.ustuck && !wont_fall && !next_to_u()) {
		    You("被你的宠物挤了上去!");
		    wont_fall = TRUE;
		}

		/* Floor objects get a chance of falling down.  The case where
		 * the hero does NOT fall down is treated here.  The case
		 * where the hero does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
		    if (newobjs)
			impact_drop((struct obj *)0, x, y, 0);
		    if (oldobjs != newobjs)
			(void) pickup(1);
		    if (shopdoor && madeby_u) pay_for_damage("破坏地面", FALSE);

		} else {
		    d_level newlevel;
		    const char *You_fall = "你顺着洞掉了下去...";

		    if (*u.ushops && madeby_u)
			shopdig(1); /* shk might snatch pack */
		    /* handle earlier damage, eg breaking wand of digging */
		    else if (!madeby_u) pay_for_damage("乱挖地面", TRUE);

		    /* Earlier checks must ensure that the destination
		     * level exists and is in the present dungeon.
		     */
		    newlevel.dnum = u.uz.dnum;
		    newlevel.dlevel = u.uz.dlevel + 1;
		    /* Cope with holes caused by monster's actions -- ALI */
		    if (flags.mon_moving) {
			schedule_goto(&newlevel, FALSE, TRUE, FALSE,
			  You_fall, (char *)0);
		    } else {
			pline(You_fall);
		    goto_level(&newlevel, FALSE, TRUE, FALSE);
		    /* messages for arriving in special rooms */
		    spoteffects(FALSE);
		}
		}
	    } else {
		if (shopdoor && madeby_u) pay_for_damage("破坏门", FALSE);
		if (newobjs)
		    impact_drop((struct obj *)0, x, y, 0);
		if (mtmp) {
		     /*[don't we need special sokoban handling here?]*/
		    if (is_flyer(mtmp->data) || is_floater(mtmp->data) ||
		        mtmp->data == &mons[PM_WUMPUS] ||
			(mtmp->wormno && count_wsegs(mtmp) > 5) ||
			mtmp->data->msize >= MZ_HUGE) return;
		    if (mtmp == u.ustuck)	/* probably a vortex */
			    return;		/* temporary? kludge */

		    if (teleport_pet(mtmp, FALSE)) {
			d_level tolevel;

			if (Is_stronghold(&u.uz)) {
			    assign_level(&tolevel, &valley_level);
			} else if (Is_botlevel(&u.uz)) {
			    if (canseemon(mtmp))
				pline("%s躲开了那个陷阱.", Monnam(mtmp));
			    return;
			} else {
			    get_level(&tolevel, depth(&u.uz) + 1);
			}
			if (mtmp->isshk) make_angry_shk(mtmp, 0, 0);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
		    }
		}
	    }
	}
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
dighole(pit_only)
boolean pit_only;
{
	register struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) ||
	   /* ALI - artifact doors */
	   IS_DOOR(levl[u.ux][u.uy].typ) && artifact_door(u.ux, u.uy) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("这边的%s实在是挖不动.", surface(u.ux,u.uy));

	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		pline_The("%s被你搞出了一点波浪，但是不一会就恢复回去了.",
			is_lava(u.ux, u.uy) ? "岩浆" : "水池");
		wake_nearby();	/* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (pit_only) {
		    pline_The("吊桥你根本就挖不动.");
		    return FALSE;
	} else if (IS_GRAVE(lev->typ)) {        
	    digactualhole(u.ux, u.uy, BY_YOU, PIT);
	    dig_up_grave();
	    return TRUE;
		} else {
		    int x = u.ux, y = u.uy;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&x, &y);
		    destroy_drawbridge(x, y);
		    return TRUE;
		}

	} else if ((boulder_here = sobj_at(BOULDER, u.ux, u.uy)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
		    rn2(2)) {
			pline_The("石头掉进了坑.");
			ttmp->ttyp = PIT;	 /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
			pline("咔嚓!石头掉进去了!");
			(void) delfloortrap(ttmp);
		}
		delobj(boulder_here);
		return TRUE;

	} else if (IS_GRAVE(lev->typ)) {        
	    dig_up_grave();
			digactualhole(u.ux, u.uy, BY_YOU, PIT);
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
			pline_The("这边的%s实在是太难挖掘了.",
			      surface(u.ux, u.uy));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(u.ux, u.uy);

		pline("在你挖掘的时候周围的%s涌了进来!",
		      typ == LAVAPOOL ? "岩浆" : "水");
		/* KMH, balance patch -- new intrinsic */
		if (!Levitation && !Flying) {
		    if (typ == LAVAPOOL)
			(void) lava_effects();
		    else if (!Wwalking && !Swimming)
			(void) drown();
		}
		return TRUE;

	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
		pline_The("王座实在太难打碎了.");

	} else if (IS_ALTAR(lev->typ)) {
		pline_The("祭坛非常难打碎.");

	} else {
		typ = fillholetyp(u.ux,u.uy);

		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(u.ux, u.uy, BY_YOU, PIT);
		else
			digactualhole(u.ux, u.uy, BY_YOU, HOLE);

		return TRUE;
	}

	return FALSE;
}

STATIC_OVL void
dig_up_grave()
{
	struct obj *otmp;

	/* Grave-robbing is frowned upon... */
	exercise(A_WIS, FALSE);
	if (Role_if(PM_ARCHEOLOGIST)) {
	    adjalign(-sgn(u.ualign.type)*3);
	    You_feel("感觉像是个盗墓贼!");
	} else if (Role_if(PM_SAMURAI)) {
	    adjalign(-sgn(u.ualign.type));
	    You("惊扰了光荣的死者!");
	} else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
	    adjalign(-sgn(u.ualign.type));
	    You("打破了这片神圣的墓地的平静!");
	}

	switch (rn2(5)) {
	case 0:
	case 1:
	    You("挖出来了一具尸体.");
	    if (!!(otmp = mk_tt_object(CORPSE, u.ux, u.uy)))
	    	otmp->age -= 100;		/* this is an *OLD* corpse */;
	    break;
	case 2:
	    if (!Blind) pline(Hallucination ? "妈呀!僵尸!" :
 			"墓主非常生气!");
 	    (void) makemon(mkclass(S_ZOMBIE,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	case 3:
	    if (!Blind) pline(Hallucination ? "妈妈!我要麻麻!" :
 			"你惊扰了坟墓!");
 	    (void) makemon(mkclass(S_MUMMY,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	default:
	    /* No corpse */
	    pline_The("这个坟墓里头什么都没有?真奇怪....");
	    break;
	}
	levl[u.ux][u.uy].typ = ROOM;
	del_engr_at(u.ux, u.uy);
	newsym(u.ux,u.uy);
	return;
}

int
use_pick_axe(obj)
struct obj *obj;
{
	boolean ispick;
	char dirsyms[12];
	char qbuf[QBUFSZ];
	register char *dsp = dirsyms;
	register int rx, ry;
	int res = 0;
	register const char *sdp, *verb;

	if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

	/* Check tool */
	if (obj != uwep) {
	    if (!wield_tool(obj, "挥舞")) return 0;
	    else res = 1;
	}
	ispick = is_pick(obj);
	verb = ispick ? "挖掘" : "劈砍";

	if (u.utrap && u.utraptype == TT_WEB) {
	    pline("%s你在被卡在网里头的时候没法%s.",
		  /* res==0 => no prior message;
		     res==1 => just got "You now wield a pick-axe." message */
		  !res ? "倒霉的是," : "但是", verb);
	    return res;
	}

	while(*sdp) {
		(void) movecmd(*sdp);	/* sets u.dx and u.dy and u.dz */
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		/* Include down even with axe, so we have at least one direction */
		if (u.dz > 0 ||
		    (u.dz == 0 && isok(rx, ry) &&
		     dig_typ(obj, rx, ry) != DIGTYP_UNDIGGABLE))
			*dsp++ = *sdp;
		sdp++;
	}
	*dsp = 0;
	Sprintf(qbuf, "你想要%s哪个地方? [%s]", verb, dirsyms);
	if(!getdir(qbuf))
		return(res);

	return (use_pick_axe2(obj));
}

/* general dig through doors/etc. function
 * Handles pickaxes/lightsabers/axes
 * called from doforce and use_pick_axe
 */

/* MRKR: use_pick_axe() is split in two to allow autodig to bypass */
/*       the "In what direction do you want to dig?" query.        */
/*       use_pick_axe2() uses the existing u.dx, u.dy and u.dz    */

int use_pick_axe2(obj)
struct obj *obj;
{
	register int rx, ry;
	register struct rm *lev;
	int dig_target, digtyp;
	boolean ispick = is_pick(obj);
	const char *verbing = ispick ? "挖掘" :
#ifdef LIGHTSABERS
		is_lightsaber(uwep) ? "切削" :
#endif
		"劈砍";

	/* 0 = pick, 1 = lightsaber, 2 = axe */
	digtyp = (is_pick(uwep) ? 0 :
#ifdef LIGHTSABERS
		is_lightsaber(uwep) ? 1 :
#endif
		2);

	if (u.uswallow && attack(u.ustuck)) {
		;  /* return(1) */
	} else if (Underwater) {
		pline("周围的水阻力抵消了你的%s举动.", verbing);
	} else if(u.dz < 0) {
		if(Levitation)
		    if (digtyp == 1)
			pline_The("天花板太难挖了.");
		    else
			You("不够高.");
		else
			You_cant("没法碰到%s.",ceiling(u.ux,u.uy));
	} else if(!u.dx && !u.dy && !u.dz) {
		/* NOTREACHED for lightsabers/axes called from doforce */
		
		char buf[BUFSZ];
		int dam;

		dam = rnd(2) + dbon() + obj->spe;
		if (dam <= 0) dam = 1;
		You("用%s打你自己.", yname(uwep));
		Sprintf(buf, "%s 的 %s", uhis(),
				OBJ_NAME(objects[obj->otyp]));
		losehp(dam, buf, KILLED_BY);
		flags.botl=1;
		return(1);
	} else if(u.dz == 0) {
		if(Stunned || (Confusion && !rn2(5))) confdir();
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(!isok(rx, ry)) {
			if (digtyp == 1) pline("你的%s从身上轻飘飘的弹开.",
				aobjnam(obj, (char *)0));
			else pline("咔嚓!");
			return(1);
		}
		lev = &levl[rx][ry];
		if(MON_AT(rx, ry) && attack(m_at(rx, ry)))
			return(1);
		dig_target = dig_typ(obj, rx, ry);
		if (dig_target == DIGTYP_UNDIGGABLE) {
			/* ACCESSIBLE or POOL */
			struct trap *trap = t_at(rx, ry);

			if (trap && trap->ttyp == WEB) {
			    if (!trap->tseen) {
				seetrap(trap);
				There("有个蜘蛛网!");
			    }
			    Your("%s困在了网里头.",
				aobjnam(obj, "被"));
			    /* you ought to be able to let go; tough luck */
			    /* (maybe `move_into_trap()' would be better) */
			    nomul(-d(2,2));
			    nomovemsg = "成功的挣扎出来.";
			} else if (lev->typ == IRONBARS) {
			    pline("碰!");
			    wake_nearby();
			} else if (IS_TREE(lev->typ))
			    You("需要一把斧头才能砍树.");
			else if (IS_ROCK(lev->typ))
			    You("需要一把镐子才能挖掘.");
			else if (!ispick && (sobj_at(STATUE, rx, ry) ||
					     sobj_at(BOULDER, rx, ry))) {
			    boolean vibrate = !rn2(3);
			    pline("在你试图砍%s的时候%s",
				sobj_at(STATUE, rx, ry) ? "雕像" : "巨石",
				vibrate ? " 你手里的斧头被震的乱抖!" : "");
			    if (vibrate) losehp(2, "试图用斧头砍石头", KILLED_BY);
			}
			else
			    You("在空气中挥舞你的%s.",
				aobjnam(obj, (char *)0));
		} else {
			static const char * const d_action[6][2] = {
			    {"挥舞","劈砍空气"},
			    {"挖掘","劈砍墙壁"},
			    {"劈砍雕像","切削雕像"},
			    {"击打巨石","切削巨石"},
			    {"砍门","用火烧门"},
			    {"砍树","用火烧树"}
			};
			did_dig_msg = FALSE;
			digging.quiet = FALSE;
			if (digging.pos.x != rx || digging.pos.y != ry ||
			    !on_level(&digging.level, &u.uz) || digging.down) {
			    if (flags.autodig &&
				dig_target == DIGTYP_ROCK && !digging.down &&
				digging.pos.x == u.ux &&
				digging.pos.y == u.uy &&
				(moves <= digging.lastdigtime+2 &&
				 moves >= digging.lastdigtime)) {
				/* avoid messages if repeated autodigging */
				did_dig_msg = TRUE;
				digging.quiet = TRUE;
			    }
			    digging.down = digging.chew = FALSE;
			    digging.warned = FALSE;
			    digging.pos.x = rx;
			    digging.pos.y = ry;
			    assign_level(&digging.level, &u.uz);
			    digging.effort = 0;
			    if (!digging.quiet)
				You("开始%s.", d_action[dig_target][digtyp == 1]);
			} else {
			    You("%s %s.", digging.chew ? "开始" : "继续",
					d_action[dig_target][digtyp == 1]);
			    digging.chew = FALSE;
			}
			set_occupation(dig, verbing, 0);
		}
	} else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
		You("在空气中挥舞你的%s.", aobjnam(obj, (char *)0));
	} else if (!can_reach_floor()) {
		You_cant("碰到地板%s.", surface(u.ux,u.uy));
	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		/* Monsters which swim also happen not to be able to dig */
		You("没法在%s里头停留这么久.",
				is_pool(u.ux, u.uy) ? "水" : " 岩浆");
	} else if (digtyp == 2) {
		Your("的%s在%s上面只留下了点划痕.",
				aobjnam(obj, (char *)0), surface(u.ux,u.uy));
		u_wipe_engr(3);
	} else {
		if (digging.pos.x != u.ux || digging.pos.y != u.uy ||
			!on_level(&digging.level, &u.uz) || !digging.down) {
		    digging.chew = FALSE;
		    digging.down = TRUE;
		    digging.warned = FALSE;
		    digging.pos.x = u.ux;
		    digging.pos.y = u.uy;
		    assign_level(&digging.level, &u.uz);
		    digging.effort = 0;
		    You("开始向下%s.", verbing);
		    if (*u.ushops) shopdig(0);
		} else
		    You("接着向下%s.", verbing);
		did_dig_msg = FALSE;
		set_occupation(dig, verbing, 0);
	}
	return(1);
}

/*
 * Town Watchmen frown on damage to the town walls, trees or fountains.
 * It's OK to dig holes in the ground, however.
 * If mtmp is assumed to be a watchman, a watchman is found if mtmp == 0
 * zap == TRUE if wand/spell of digging, FALSE otherwise (chewing)
 */
void
watch_dig(mtmp, x, y, zap)
    struct monst *mtmp;
    xchar x, y;
    boolean zap;
{
	struct rm *lev = &levl[x][y];

	if (in_town(x, y) &&
	    (closed_door(x, y) || lev->typ == SDOOR ||
	     IS_WALL(lev->typ) || IS_FOUNTAIN(lev->typ) || IS_TREE(lev->typ))) {
	    if (!mtmp) {
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if ((mtmp->data == &mons[PM_WATCHMAN] ||
			 mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			mtmp->mcansee && m_canseeu(mtmp) &&
			couldsee(mtmp->mx, mtmp->my) && mtmp->mpeaceful)
			break;
		}
	    }

	    if (mtmp) {
		if(zap || digging.warned) {
		    verbalize("喂！破坏者！你被捕了!停下！");
		    (void) angry_guards(!(flags.soundok));
		} else {
		    const char *str;

		    if (IS_DOOR(lev->typ))
			str = "门";
		    else if (IS_TREE(lev->typ))
			str = "树";
		    else if (IS_ROCK(lev->typ))
			str = "墙";
		    else
			str = "喷泉";
		    verbalize("喂!不要再破坏那个%s了!", str);
		    digging.warned = TRUE;
		}
		if (is_digging())
		    stop_occupation();
	    }
	}
}

#endif /* OVLB */
#ifdef OVL0

/* Return TRUE if monster died, FALSE otherwise.  Called from m_move(). */
boolean
mdig_tunnel(mtmp)
register struct monst *mtmp;
{
	register struct rm *here;
	int pile = rnd(12);

	here = &levl[mtmp->mx][mtmp->my];
	if (here->typ == SDOOR)
	    cvt_sdoor_to_door(here);	/* ->typ = DOOR */

	/* Eats away door if present & closed or locked */
	if (closed_door(mtmp->mx, mtmp->my)) {
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */
	    if (here->doormask & D_TRAPPED) {
		here->doormask = D_NODOOR;
		if (mb_trapped(mtmp)) {	/* mtmp is killed */
		    newsym(mtmp->mx, mtmp->my);
		    return TRUE;
		}
	    } else {
		if (!rn2(3) && flags.verbose)	/* not too often.. */
		    You_feel("一种奇怪的轻松感.");
		here->doormask = D_BROKEN;
	    }
	    newsym(mtmp->mx, mtmp->my);
	    return FALSE;
	} else if (!IS_ROCK(here->typ) && !IS_TREE(here->typ)) /* no dig */
	    return FALSE;

	/* Only rock, trees, and walls fall through to this point. */
	if ((here->wall_info & W_NONDIGGABLE) != 0) {
	    impossible("错误：挖掘的%s不可挖掘",
		       (IS_WALL(here->typ) ? "墙" : "石头"),
		       (int) mtmp->mx, (int) mtmp->my);
	    return FALSE;	/* still alive */
	}

	if (IS_WALL(here->typ)) {
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
	    if (flags.soundok && flags.verbose && !rn2(5))
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
		You_hear("石头的破碎声.");
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    if (level.flags.is_maze_lev) {
		here->typ = ROOM;
	    } else if (level.flags.is_cavernous_lev &&
		       !in_town(mtmp->mx, mtmp->my)) {
		here->typ = CORR;
	    } else {
		here->typ = DOOR;
		here->doormask = D_NODOOR;
	    }
	} else if (IS_TREE(here->typ)) {
	    here->typ = ROOM;
	    if (pile && pile < 5)
		(void) rnd_treefruit_at(mtmp->mx, mtmp->my);
	} else {
	    here->typ = CORR;
	    if (pile && pile < 5)
	    (void) mksobj_at((pile == 1) ? BOULDER : ROCK,
			     mtmp->mx, mtmp->my, TRUE, FALSE);
	}
	newsym(mtmp->mx, mtmp->my);
	if (!sobj_at(BOULDER, mtmp->mx, mtmp->my))
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */

	return FALSE;
}

#endif /* OVL0 */
#ifdef OVL3

/* digging via wand zap or spell cast */
void
zap_dig()
{
	struct rm *room;
	struct monst *mtmp;
/*        struct obj *otmp;*/
        register struct obj *otmp, *next_obj;
	int zx, zy, digdepth;
	boolean shopdoor, shopwall, maze_dig;
	/*
	 * Original effect (approximately):
	 * from CORR: dig until we pierce a wall
	 * from ROOM: pierce wall and dig until we reach
	 * an ACCESSIBLE place.
	 * Currently: dig for digdepth positions;
	 * also down on request of Lennart Augustsson.
	 */

	if (u.uswallow) {
	    mtmp = u.ustuck;

	    if (!is_whirly(mtmp->data)) {
		if (is_animal(mtmp->data))
		    You("刺穿了%s的%s壁!",
			s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
		mtmp->mhp = 1;		/* almost dead */
		expels(mtmp, mtmp->data, !is_animal(mtmp->data));
	    }
	    return;
	} /* swallowed */

	if (u.dz) {
	    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !Underwater) {
		if (u.dz < 0 || On_stairs(u.ux, u.uy)) {
		    if (On_stairs(u.ux, u.uy))
			pline_The("射线从%s上反弹了下来后打中了%s.",
			      (u.ux == xdnladder || u.ux == xupladder) ?
			      "梯子" : "楼梯", ceiling(u.ux, u.uy));
		    You("从%s上面搞下来了一块石头.", ceiling(u.ux, u.uy));
		    pline("它砸中了你的%s!", body_part(HEAD));
		    losehp(rnd((uarmh && is_metallic(uarmh)) ? 2 : 6),
			   "掉落的石头", KILLED_BY_AN);
		    otmp = mksobj_at(ROCK, u.ux, u.uy, FALSE, FALSE);
		    if (otmp) {
			(void)xname(otmp);	/* set dknown, maybe bknown */
			stackobj(otmp);
		    }
		    newsym(u.ux, u.uy);
		} else {
		    watch_dig((struct monst *)0, u.ux, u.uy, TRUE);
		    (void) dighole(FALSE);
		}
	    }
	    return;
	} /* up or down */

	/* normal case: digging across the level */
	shopdoor = shopwall = FALSE;
	maze_dig = level.flags.is_maze_lev && !Is_earthlevel(&u.uz);
	zx = u.ux + u.dx;
	zy = u.uy + u.dy;
	digdepth = rn1(18, 8);
	tmp_at(DISP_BEAM, cmap_to_glyph(S_digbeam));
	while (--digdepth >= 0) {
	    if (!isok(zx,zy)) break;
	    room = &levl[zx][zy];
	    tmp_at(zx,zy);
	    delay_output();	/* wait a little bit */

            /* WAC check for monster, boulder */
            if ((mtmp = m_at(zx, zy)) != 0) {
                if (made_of_rock(mtmp->data)) {
                    You("在%s里头挖了个洞!", mon_nam(mtmp));
                    mtmp->mhp /= 2;
                    if (mtmp->mhp < 1) mtmp->mhp = 1;
		    setmangry(mtmp);
                } else pline("%s并未被影响!", Monnam(mtmp));
            }
            for(otmp = level.objects[zx][zy]; otmp; otmp = next_obj) {
                next_obj = otmp->nexthere;
		/* vaporize boulders */
                if (otmp->otyp == BOULDER) {
		    delobj(otmp);
		    /* A little Sokoban guilt... */
		    if (In_sokoban(&u.uz))
			change_luck(-1);
		    unblock_point(zx, zy);
		    newsym(zx, zy);
		    pline_The("巨石被气化了!");
		}
		break;
            }

	    if (closed_door(zx, zy) || room->typ == SDOOR) {
		/* ALI - Artifact doors */
		if (artifact_door(zx, zy)) {
		    if (cansee(zx, zy))
			pline_The("门亮了一下，然后又暗下去了.");
		    break;
		}
		if (*in_rooms(zx,zy,SHOPBASE)) {
		    add_damage(zx, zy, 400L);
		    shopdoor = TRUE;
		}
		if (room->typ == SDOOR)
		    room->typ = DOOR;
		else if (cansee(zx, zy))
		    pline_The("门被推平了!");
		watch_dig((struct monst *)0, zx, zy, TRUE);
		room->doormask = D_NODOOR;
		unblock_point(zx,zy); /* vision */
		digdepth -= 2;
		if (maze_dig) break;
	    } else if (maze_dig) {
		if (IS_WALL(room->typ)) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			if (*in_rooms(zx,zy,SHOPBASE)) {
			    add_damage(zx, zy, 200L);
			    shopwall = TRUE;
			}
			room->typ = ROOM;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The("墙发光了一下，但是没有事发生.");
		    break;
		} else if (IS_TREE(room->typ)) { /* check trees before stone */
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			room->typ = ROOM;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The("树颤抖了一下，但是没有事情发生.");
		    break;
		} else if (room->typ == STONE || room->typ == SCORR) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			room->typ = CORR;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The("石头发光了一小会后暗下去了.");
		    break;
		}
	    } else if (IS_ROCK(room->typ)) {
		if (!may_dig(zx,zy)) break;
		if (IS_WALL(room->typ) || room->typ == SDOOR) {
		    if (*in_rooms(zx,zy,SHOPBASE)) {
			add_damage(zx, zy, 200L);
			shopwall = TRUE;
		    }
		    watch_dig((struct monst *)0, zx, zy, TRUE);
		    if (level.flags.is_cavernous_lev && !in_town(zx, zy)) {
			room->typ = CORR;
		    } else {
			room->typ = DOOR;
			room->doormask = D_NODOOR;
		    }
		    digdepth -= 2;
		} else if (IS_TREE(room->typ)) {
		    room->typ = ROOM;
		    digdepth -= 2;
		} else {	/* IS_ROCK but not IS_WALL or SDOOR */
		    room->typ = CORR;
		    digdepth--;
		}
		unblock_point(zx,zy); /* vision */
	    }
	    zx += u.dx;
	    zy += u.dy;
	} /* while */
	tmp_at(DISP_END,0);	/* closing call */
	if (shopdoor || shopwall)
	    pay_for_damage(shopdoor ? "摧毁" : "挖穿", FALSE);
	return;
}

/* move objects from fobj/nexthere lists to buriedobjlist, keeping position */
/* information */
struct obj *
bury_an_obj(otmp)
	struct obj *otmp;
{
	struct obj *otmp2;
	boolean under_ice;

#ifdef DEBUG
	pline("埋一个物品: %s", xname(otmp));
#endif
	if (otmp == uball)
		unpunish();
	/* after unpunish(), or might get deallocated chain */
	otmp2 = otmp->nexthere;
	/*
	 * obj_resists(,0,0) prevents Rider corpses from being buried.
	 * It also prevents The Amulet and invocation tools from being
	 * buried.  Since they can't be confined to bags and statues,
	 * it makes sense that they can't be buried either, even though
	 * the real reason there (direct accessibility when carried) is
	 * completely different.
	 */
	if (otmp == uchain || obj_resists(otmp, 0, 0))
		return(otmp2);

	if (otmp->otyp == LEASH && otmp->leashmon != 0)
		o_unleash(otmp);

#ifdef STEED
	if (otmp == usaddle)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	if (otmp->lamplit && otmp->otyp != POT_OIL)
		end_burn(otmp, TRUE);

	obj_extract_self(otmp);

	under_ice = is_ice(otmp->ox, otmp->oy);
	if (otmp->otyp == ROCK && !under_ice) {
		/* merges into burying material */
		obfree(otmp, (struct obj *)0);
		return(otmp2);
	}
	/*
	 * Start a rot on organic material.  Not corpses -- they
	 * are already handled.
	 */
	if (otmp->otyp == CORPSE) {
	    ;		/* should cancel timer if under_ice */
	} else if ((under_ice ? otmp->oclass == POTION_CLASS : is_organic(otmp))
		&& !obj_resists(otmp, 5, 95)) {
	    (void) start_timer((under_ice ? 0L : 250L) + (long)rnd(250),
			       TIMER_OBJECT, ROT_ORGANIC, (genericptr_t)otmp);
	}
	add_to_buried(otmp);
	return(otmp2);
}

void
bury_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	if(level.objects[x][y] != (struct obj *)0)
		pline("埋起来的物品 at %d, %d", x, y);
#endif
	for (otmp = level.objects[x][y]; otmp; otmp = otmp2)
		otmp2 = bury_an_obj(otmp);

	/* don't expect any engravings here, but just in case */
	del_engr_at(x, y);
	newsym(x, y);
}

/* move objects from buriedobjlist to fobj/nexthere lists */
void
unearth_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	pline("被挖出来的物品: at %d, %d", x, y);
#endif
	for (otmp = level.buriedobjlist; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (otmp->ox == x && otmp->oy == y) {
		    obj_extract_self(otmp);
		    if (otmp->timed)
			(void) stop_timer(ROT_ORGANIC, (genericptr_t)otmp);
		    place_object(otmp, x, y);
		    stackobj(otmp);
		}
	}
	del_engr_at(x, y);
	newsym(x, y);
}

/*
 * The organic material has rotted away while buried.  As an expansion,
 * we could add add partial damage.  A damage count is kept in the object
 * and every time we are called we increment the count and reschedule another
 * timeout.  Eventually the object rots away.
 *
 * This is used by buried objects other than corpses.  When a container rots
 * away, any contents become newly buried objects.
 */
/* ARGSUSED */
void
rot_organic(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
#if defined(MAC_MPW)
# pragma unused ( timeout )
#endif
	struct obj *obj = (struct obj *) arg;

	while (Has_contents(obj)) {
	    /* We don't need to place contained object on the floor
	       first, but we do need to update its map coordinates. */
	    obj->cobj->ox = obj->ox,  obj->cobj->oy = obj->oy;
	    /* Everything which can be held in a container can also be
	       buried, so bury_an_obj's use of obj_extract_self insures
	       that Has_contents(obj) will eventually become false. */
	    (void)bury_an_obj(obj->cobj);
	}
	obj_extract_self(obj);
	obfree(obj, (struct obj *) 0);
}

/*
 * Called when a corpse has rotted completely away.
 */
void
rot_corpse(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
	xchar x = 0, y = 0;
	struct obj *obj = (struct obj *) arg;
	boolean on_floor = obj->where == OBJ_FLOOR,
                in_minvent = obj->where == OBJ_MINVENT,
		in_invent = obj->where == OBJ_INVENT;

	if (on_floor) {
	    x = obj->ox;
	    y = obj->oy;
        } else if (in_minvent) {
            /* WAC unwield if wielded */
            if (MON_WEP(obj->ocarry) && MON_WEP(obj->ocarry) == obj) {
		    obj->owornmask &= ~W_WEP;
                    MON_NOWEP(obj->ocarry);
	    }
	} else if (in_invent) {
	    if (flags.verbose) {
		char *cname = corpse_xname(obj, FALSE);
		Your("%s%s %s掉了%c",
		     obj == uwep ? "拿着的 " : nul, cname,
		     otense(obj, "腐烂"), obj == uwep ? '!' : '.');
	    }
	    if (obj == uwep) {
		uwepgone();	/* now bare handed */
		stop_occupation();
	    } else if (obj == uswapwep) {
		uswapwepgone();
		stop_occupation();
	    } else if (obj == uquiver) {
		uqwepgone();
		stop_occupation();
	    } else if (obj == uswapwep) {
		uswapwepgone();
		stop_occupation();
	    } else if (obj == uquiver) {
		uqwepgone();
		stop_occupation();
	    }
	} else if (obj->where == OBJ_MINVENT && obj->owornmask) {
	    if (obj == MON_WEP(obj->ocarry)) {
		setmnotwielded(obj->ocarry,obj);
		MON_NOWEP(obj->ocarry);
	    }
	}
	rot_organic(arg, timeout);
	if (on_floor) newsym(x, y);
	else if (in_invent) update_inventory();
}

#if 0
void
bury_monst(mtmp)
struct monst *mtmp;
{
#ifdef DEBUG
	pline("活埋怪物: %s", mon_nam(mtmp));
#endif
	if(canseemon(mtmp)) {
	    if(is_flyer(mtmp->data) || is_floater(mtmp->data)) {
		pline_The("%s突然裂开一个口子,但是%s没有掉进去!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
		return;
	    } else
	        pline_The("%s裂开一个口子后%s掉进去了!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
	}

	mtmp->mburied = TRUE;
	wakeup(mtmp);			/* at least give it a chance :-) */
	newsym(mtmp->mx, mtmp->my);
}

void
bury_you()
{
#ifdef DEBUG
	pline("把你埋了");
#endif
	/* KMH, balance patch -- new intrinsic */
    if (!Levitation && !Flying) {
	if(u.uswallow)
	    You_feel("感觉像是要掉进陷阱里了!");
	else
	    pline_The("%s突然裂开一个口子后你掉进去了!",
		  surface(u.ux, u.uy));

	u.uburied = TRUE;
	if(!Strangled && !Breathless) Strangled = 4;
	under_ground(1);
    }
}

void
unearth_you()
{
#ifdef DEBUG
	pline("把你挖出来");
#endif
	u.uburied = FALSE;
	under_ground(0);
	if(!uamul || uamul->otyp != AMULET_OF_STRANGULATION)
		Strangled = 0;
	vision_recalc(0);
}

void
escape_tomb()
{
#ifdef DEBUG
	pline("逃离坟墓");
#endif
	if ((Teleportation || can_teleport(youmonst.data)) &&
	    (Teleport_control || rn2(3) < Luck+2)) {
		You("试图释放一个传送魔法.");
		(void) dotele();	/* calls unearth_you() */
	} else if(u.uburied) { /* still buried after 'port attempt */
		boolean good;

		if(amorphous(youmonst.data) || Passes_walls ||
		   noncorporeal(youmonst.data) || unsolid(youmonst.data) ||
		   (tunnels(youmonst.data) && !needspick(youmonst.data))) {

		    You("从坑里头试图%s岩石到%s上.",
			(tunnels(youmonst.data) && !needspick(youmonst.data)) ?
			 "尝试挖掘" : (amorphous(youmonst.data)) ?
			 "渗透" : "穿过", surface(u.ux, u.uy));

		    if(tunnels(youmonst.data) && !needspick(youmonst.data))
			good = dighole(TRUE);
		    else good = TRUE;
		    if(good) unearth_you();
		}
	}
}

void
bury_obj(otmp)
struct obj *otmp;
{

#ifdef DEBUG
	pline("埋");
#endif
	if(cansee(otmp->ox, otmp->oy))
	   pline_The("在%s上头的东西突然掉进一个洞里头了!",
		surface(otmp->ox, otmp->oy));

	bury_objs(otmp->ox, otmp->oy);
}
#endif

#ifdef DEBUG
int
wiz_debug_cmd() /* in this case, bury everything at your loc and around */
{
	int x, y;

	for (x = u.ux - 1; x <= u.ux + 1; x++)
	    for (y = u.uy - 1; y <= u.uy + 1; y++)
		if (isok(x,y)) bury_objs(x,y);
	return 0;
}

#endif /* DEBUG */
#endif /* OVL3 */

/*dig.c*/
