/*	SCCS Id: @(#)priest.c	3.4	2002/11/06	*/
/* Copyright (c) Izchak Miller, Steve Linhart, 1989.		  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"
#include "eshk.h"
#include "epri.h"
#include "emin.h"

/* this matches the categorizations shown by enlightenment */
#define ALGN_SINNED	(-4)	/* worse than strayed */

#ifdef OVLB

STATIC_DCL boolean FDECL(histemple_at,(struct monst *,XCHAR_P,XCHAR_P));
STATIC_DCL boolean FDECL(has_shrine,(struct monst *));

/*
 * Move for priests and shopkeepers.  Called from shk_move() and pri_move().
 * Valid returns are  1: moved  0: didn't  -1: let m_move do it  -2: died.
 */
int
move_special(mtmp,in_his_shop,appr,uondoor,avoid,omx,omy,gx,gy)
register struct monst *mtmp;
boolean in_his_shop;
schar appr;
boolean uondoor,avoid;
register xchar omx,omy,gx,gy;
{
	register xchar nx,ny,nix,niy;
	register schar i;
	schar chcnt,cnt;
	coord poss[9];
	long info[9];
	long allowflags;
	struct obj *ib = (struct obj *)0;

	if(omx == gx && omy == gy)
		return(0);
	if(mtmp->mconf) {
		avoid = FALSE;
		appr = 0;
	}

	nix = omx;
	niy = omy;
	if (mtmp->isshk) allowflags = ALLOW_SSM;
	else allowflags = ALLOW_SSM | ALLOW_SANCT;
	if (passes_walls(mtmp->data)) allowflags |= (ALLOW_ROCK|ALLOW_WALL);
	if (throws_rocks(mtmp->data)) allowflags |= ALLOW_ROCK;
	if (tunnels(mtmp->data)) allowflags |= ALLOW_DIG;
	if (!nohands(mtmp->data) && !verysmall(mtmp->data)) {
		allowflags |= OPENDOOR;
		if (m_carrying(mtmp, SKELETON_KEY)) allowflags |= BUSTDOOR;
	}
	if (is_giant(mtmp->data)) allowflags |= BUSTDOOR;
	cnt = mfndpos(mtmp, poss, info, allowflags);

	if(mtmp->isshk && avoid && uondoor) { /* perhaps we cannot avoid him */
		for(i=0; i<cnt; i++)
		    if(!(info[i] & NOTONL)) goto pick_move;
		avoid = FALSE;
	}

#define GDIST(x,y)	(dist2(x,y,gx,gy))
pick_move:
	chcnt = 0;
	for(i=0; i<cnt; i++) {
		nx = poss[i].x;
		ny = poss[i].y;
		if(levl[nx][ny].typ == ROOM ||
			(mtmp->ispriest &&
			    levl[nx][ny].typ == ALTAR) ||
			(mtmp->isshk &&
			    (!in_his_shop || ESHK(mtmp)->following))) {
		    if(avoid && (info[i] & NOTONL))
			continue;
		    if((!appr && !rn2(++chcnt)) ||
			(appr && GDIST(nx,ny) < GDIST(nix,niy))) {
			    nix = nx;
			    niy = ny;
		    }
		}
	}
	if(mtmp->ispriest && avoid &&
			nix == omx && niy == omy && onlineu(omx,omy)) {
		/* might as well move closer as long it's going to stay
		 * lined up */
		avoid = FALSE;
		goto pick_move;
	}

	if(nix != omx || niy != omy) {
		remove_monster(omx, omy);
		place_monster(mtmp, nix, niy);
		newsym(nix,niy);
		if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
		    check_special_room(FALSE);
		if(ib) {
			if (cansee(mtmp->mx,mtmp->my))
			    pline("%s把%s捡了起来。", Monnam(mtmp),
				distant_name(ib,doname));
			obj_extract_self(ib);
			(void) mpickobj(mtmp, ib);
		}
		return(1);
	}
	return(0);
}

#endif /* OVLB */

#ifdef OVL0

char
temple_occupied(array)
register char *array;
{
	register char *ptr;

	for (ptr = array; *ptr; ptr++)
		if (rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
			return(*ptr);
	return('\0');
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_OVL boolean
histemple_at(priest, x, y)
register struct monst *priest;
register xchar x, y;
{
	return((boolean)((EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE)) &&
	       on_level(&(EPRI(priest)->shrlevel), &u.uz)));
}

/*
 * pri_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
pri_move(priest)
register struct monst *priest;
{
	register xchar gx,gy,omx,omy;
	schar temple;
	boolean avoid = TRUE;

	omx = priest->mx;
	omy = priest->my;

	if(!histemple_at(priest, omx, omy)) return(-1);

	temple = EPRI(priest)->shroom;

	gx = EPRI(priest)->shrpos.x;
	gy = EPRI(priest)->shrpos.y;

	gx += rn1(3,-1);	/* mill around the altar */
	gy += rn1(3,-1);

	if(!priest->mpeaceful ||
	   (Conflict && !resist(priest, RING_CLASS, 0, 0))) {
		if(monnear(priest, u.ux, u.uy)) {
			if(Displaced)
				Your("的幻影并没有成功骗到%s的眼睛！",
					mon_nam(priest));
			(void) mattacku(priest);
			return(0);
		} else if(index(u.urooms, temple)) {
			/* chase player if inside temple & can see him */
			if(priest->mcansee && m_canseeu(priest)) {
				gx = u.ux;
				gy = u.uy;
			}
			avoid = FALSE;
		}
	} else if(Invis) avoid = FALSE;

	return(move_special(priest,FALSE,TRUE,FALSE,avoid,omx,omy,gx,gy));
}

/* exclusively for mktemple() */
void
priestini(lvl, sroom, sx, sy, sanctum)
d_level	*lvl;
struct mkroom *sroom;
int sx, sy;
boolean sanctum;   /* is it the seat of the high priest? */
{
	struct monst *priest;
	struct obj *otmp = NULL;
	int cnt;

	if(MON_AT(sx+1, sy))
		(void) rloc(m_at(sx+1, sy), FALSE); /* insurance */

	priest = makemon(&mons[sanctum ? PM_HIGH_PRIEST : PM_ALIGNED_PRIEST],
			 sx + 1, sy, NO_MM_FLAGS);
	if (priest) {
		EPRI(priest)->shroom = (sroom - rooms) + ROOMOFFSET;
		EPRI(priest)->shralign = Amask2align(levl[sx][sy].altarmask);
		EPRI(priest)->shrpos.x = sx;
		EPRI(priest)->shrpos.y = sy;
		assign_level(&(EPRI(priest)->shrlevel), lvl);
		priest->mtrapseen = ~0;	/* traps are known */
		priest->mpeaceful = 1;
		priest->ispriest = 1;
		priest->msleeping = 0;
		set_malign(priest); /* mpeaceful may have changed */

		/* now his/her goodies... */
		if(sanctum && EPRI(priest)->shralign == A_NONE &&
		     on_level(&sanctum_level, &u.uz)) {
			(void) mongets(priest, AMULET_OF_YENDOR);
		}
		/* 2 to 4 spellbooks */
		for (cnt = rn1(3,2); cnt > 0; --cnt) {
		    (void) mpickobj(priest, mkobj(SPBOOK_CLASS, FALSE));
		}
		/* [ALI] Upgrade existing robe or aquire new */
		if (rn2(2) || (otmp = which_armor(priest, W_ARM)) == 0) {
		    struct obj *obj;
		    obj = mksobj(rn2(p_coaligned(priest) ? 2 : 5) ?
			    ROBE_OF_PROTECTION : ROBE_OF_POWER, TRUE, FALSE);
		    if (p_coaligned(priest))
			uncurse(obj);
		    else
			curse(obj);
		    (void) mpickobj(priest, obj);
		    m_dowear(priest, TRUE);
		    if (!(obj->owornmask & W_ARM)) {
			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
		    } else if (otmp) {
			obj_extract_self(otmp);
			obfree(otmp, (struct obj *)0);
		    }
		}
	}
}

/*
 * Specially aligned monsters are named specially.
 *	- aligned priests with ispriest and high priests have shrines
 *		they retain ispriest and epri when polymorphed
 *	- aligned priests without ispriest and Angels are roamers
 *		they retain isminion and access epri as emin when polymorphed
 *		(coaligned Angels are also created as minions, but they
 *		use the same naming convention)
 *	- minions do not have ispriest but have isminion and emin
 *	- caller needs to inhibit Hallucination if it wants to force
 *		the true name even when under that influence
 */
char *
priestname(mon, pname)
register struct monst *mon;
char *pname;		/* caller-supplied output buffer */
{
	const char *what = Hallucination ? rndmonnam() : mon->data->mname;

	Strcpy(pname, "");
	if (mon->minvis) Strcat(pname, "隐形的");
	if (mon->ispriest || mon->data == &mons[PM_ALIGNED_PRIEST] ||
					mon->data == &mons[PM_ANGEL]) {
		/* use epri */
		if (mon->mtame && mon->data == &mons[PM_ANGEL])
			Strcat(pname, "守护");
		if (mon->data != &mons[PM_ALIGNED_PRIEST] &&
				mon->data != &mons[PM_HIGH_PRIEST]) {
			Strcat(pname, what);
			Strcat(pname, "");
		}
		if (mon->data != &mons[PM_ANGEL]) {
			if (!mon->ispriest && EPRI(mon)->renegade)
				Strcat(pname, "");
			if (mon->data == &mons[PM_HIGH_PRIEST])
				Strcat(pname, "");
			if (Hallucination)
				Strcat(pname, "大人物");
			else if (mon->female)
				Strcat(pname, "女祭司");
			else
				Strcat(pname, "祭司");
		}
		Strcat(pname, "的");
		Strcat(pname, halu_gname((int)EPRI(mon)->shralign));
		return(pname);
	}
	/* use emin instead of epri */
	Strcat(pname, what);
	Strcat(pname, "的");
	Strcat(pname, halu_gname(EMIN(mon)->min_align));
	return(pname);
}

boolean
p_coaligned(priest)
struct monst *priest;
{
	return((boolean)(u.ualign.type == ((int)EPRI(priest)->shralign)));
}

STATIC_OVL boolean
has_shrine(pri)
struct monst *pri;
{
	struct rm *lev;

	if(!pri)
		return(FALSE);
	lev = &levl[EPRI(pri)->shrpos.x][EPRI(pri)->shrpos.y];
	if (!IS_ALTAR(lev->typ) || !(lev->altarmask & AM_SHRINE))
		return(FALSE);
	return((boolean)(EPRI(pri)->shralign == Amask2align(lev->altarmask & ~AM_SHRINE)));
}

struct monst *
findpriest(roomno)
char roomno;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if(mtmp->ispriest && (EPRI(mtmp)->shroom == roomno) &&
	       histemple_at(mtmp,mtmp->mx,mtmp->my))
		return(mtmp);
	}
	return (struct monst *)0;
}

/* called from check_special_room() when the player enters the temple room */
void
intemple(roomno)
register int roomno;
{
	register struct monst *priest = findpriest((char)roomno);
	boolean tended = (priest != (struct monst *)0);
	boolean shrined, sanctum, can_speak;
	const char *msg1, *msg2;
	char buf[BUFSZ];

	if(!temple_occupied(u.urooms0)) {
	    if(tended) {
		shrined = has_shrine(priest);
		sanctum = (priest->data == &mons[PM_HIGH_PRIEST] &&
			   (Is_sanctum(&u.uz) || In_endgame(&u.uz)));
		can_speak = (priest->mcanmove && !priest->msleeping &&
			     flags.soundok);
		if (can_speak) {
		    unsigned save_priest = priest->ispriest;
		    /* don't reveal the altar's owner upon temple entry in
		       the endgame; for the Sanctum, the next message names
		       Moloch so suppress the "" for him here too */
		    if (sanctum && !Hallucination) priest->ispriest = 0;
		    pline("%s向你说道：",
			canseemon(priest) ? Monnam(priest) : "你周围有一道声音说道：");
		    priest->ispriest = save_priest;
		}
		msg2 = 0;
		if(sanctum && Is_sanctum(&u.uz)) {
		    if(priest->mpeaceful) {
			msg1 = "异教徒，尔等踏入了摩洛的至高圣地！";
			msg2 = "快滚罢！";
			priest->mpeaceful = 0;
			set_malign(priest);
		    } else
			msg1 = "汝以汝之存在玷污了这片圣地！";
		} else {
		    Sprintf(buf, "信徒啊，汝已踏入这片%s的神庙之中！",
			    !shrined ? "不洁的" : "神圣的");
		    msg1 = buf;
		}
		if (can_speak) {
		    verbalize(msg1);
		    if (msg2) verbalize(msg2);
		}
		if(!sanctum) {
		    /* !tended -> !shrined */
		    if (!shrined || !p_coaligned(priest) ||
			    u.ualign.record <= ALGN_SINNED)
			You("感到一股%s被拒之门外的感觉……",
				(!shrined) ? "" : "奇怪的");
		    else You("感觉到一股不属于这片地牢的宁静感。");
		}
	    } else {
		switch(rn2(3)) {
		  case 0: You("有一种不祥的预感……"); break;
		  case 1: You_feel("你正被某样东西监视着。"); break;
		  default: pline("一股颤抖从你的天灵感窜到%s。",
			body_part(SPINE)); break;
		}
		if(!rn2(5)) {
		    struct monst *mtmp;

		    if(!(mtmp = makemon(&mons[PM_GHOST],u.ux,u.uy,NO_MM_FLAGS)))
			return;
		    if (!Blind || sensemon(mtmp))
			pline("一个巨大的冤魂突然在你身边出现！");
		    else You("感觉你的边上出现了一个不明物体！");
		    mtmp->mpeaceful = 0;
		    set_malign(mtmp);
		    if(flags.verbose)
			You("被吓了个半死，根本挪不动腿。");
		    nomul(-3);
		    nomovemsg = "你勉强重新镇定了下来。";
	       }
	   }
       }
}

void
priest_talk(priest)
register struct monst *priest;
{
	boolean coaligned = p_coaligned(priest);
	boolean strayed = (u.ualign.record < 0);

	/* KMH, conduct */
	u.uconduct.gnostic++;

	if(priest->mflee || (!priest->ispriest && coaligned && strayed)) {
	    pline("%s不想和你有任何意义上的交流！",
				Monnam(priest));
	    priest->mpeaceful = 0;
	    return;
	}

	/* priests don't chat unless peaceful and in their own temple */
	if(!histemple_at(priest,priest->mx,priest->my) ||
		 !priest->mpeaceful || !priest->mcanmove || priest->msleeping) {
	    static const char *cranky_msg[3] = {
		"汝不懂圣地的道理？那么贫僧便教你一些物理！",
		"汝想交谈？那这就是吾要给你的肢体语言！",
		"朝圣者，吾不愿再与汝等有半分瓜葛。"
	    };

	    if(!priest->mcanmove || priest->msleeping) {
		pline("%s突然从%s所处的梦境中惊醒！",
		      Monnam(priest), mhis(priest));
		priest->mfrozen = priest->msleeping = 0;
		priest->mcanmove = 1;
	    }
	    priest->mpeaceful = 0;
	    verbalize(cranky_msg[rn2(3)]);
	    return;
	}

	/* you desecrated the temple and now you want to chat? */
	if(priest->mpeaceful && *in_rooms(priest->mx, priest->my, TEMPLE) &&
		  !has_shrine(priest)) {
	    verbalize("滚出吾等的圣地！汝以汝之存在玷污了这片净土。");
	    priest->mpeaceful = 0;
	    return;
	}
#ifndef GOLDOBJ
	if(!u.ugold) {
	    if(coaligned && !strayed) {
		if (priest->mgold > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
		    pline("%s塞给你%s，打发你去买瓶麦芽酒喝。", Monnam(priest),
			(priest->mgold == 1L) ? "一个钢镚儿" : "俩个钢镚儿");
		    if (priest->mgold > 1L)
			u.ugold = 2L;
		    else
			u.ugold = 1L;
		    priest->mgold -= u.ugold;
		    flags.botl = 1;
#else
	if(!money_cnt(invent)) {
	    if(coaligned && !strayed) {
                long pmoney = money_cnt(priest->minvent);
		if (pmoney > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
		    pline("%s塞给你%s，打发你去买瓶麦芽酒喝。", Monnam(priest),
			(pmoney == 1L) ? "一个钢镚儿" : "俩个钢镚儿");
		     money2u(priest, pmoney > 1L ? 2 : 1);
#endif
		} else
		    pline("%s向你宣扬当个穷人也不是什么坏事。", Monnam(priest));
		exercise(A_WIS, TRUE);
	    } else
		pline("%s对你不感任何兴趣。", Monnam(priest));
	    return;
	} else {
	    long offer;

	    pline("%s问你要不要为神庙建设捐点钱。",
			Monnam(priest));
	    if((offer = bribe(priest)) == 0) {
		verbalize("汝将为汝之行为追悔莫及！");
		if(coaligned) adjalign(-1);
	    } else if(offer < (u.ulevel * 200)) {
#ifndef GOLDOBJ
		if(u.ugold > (offer * 2L)) verbalize("你这小气鬼铁公鸡葛朗台。");
#else
		if(money_cnt(invent) > (offer * 2L)) verbalize("你这小气鬼铁公鸡葛朗台。");
#endif
		else {
		    verbalize("吾感谢汝等为吾等大业做出的贡献。");
		    /*  give player some token  */
		    exercise(A_WIS, TRUE);
		}
	    } else if(offer < (u.ulevel * 400)) {
		verbalize("汝的确是一位十分虔诚的信徒。");
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L)) {
#else
		if(money_cnt(invent) < (offer * 2L)) {
#endif
		    if (coaligned && u.ualign.record <= ALGN_SINNED)
			adjalign(1);
		    verbalize("吾将赐予汝等一个祝福。");
		    /* KMH, intrinsic patch */
		    incr_itimeout(&HClairvoyant, rn1(500,500));
		}
	    } else if(offer < (u.ulevel * 600) &&
		      u.ublessed < 20 &&
		      (u.ublessed < 9 || !rn2(u.ublessed))) {
		verbalize("吾将赐予你一份礼物，虔信者。");
		if (!(HProtection & INTRINSIC))  {
			HProtection |= FROMOUTSIDE;
			if (!u.ublessed)  u.ublessed = rn1(3, 2);
		} else u.ublessed++;
	    } else {
		verbalize("吾深深感激汝等的无私奉献精神。");
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L) && coaligned) {
#else
		if(money_cnt(invent) < (offer * 2L) && coaligned) {
#endif
		    if(strayed && (moves - u.ucleansed) > 5000L) {
			u.ualign.record = 0; /* cleanse thee */
			u.ucleansed = moves;
		    } else {
			adjalign(2);
		    }
		}
	    }
	}
}

struct monst *
mk_roamer(ptr, alignment, x, y, peaceful)
register struct permonst *ptr;
aligntyp alignment;
xchar x, y;
boolean peaceful;
{
	register struct monst *roamer;
	register boolean coaligned = (u.ualign.type == alignment);

	if (ptr != &mons[PM_ALIGNED_PRIEST] && ptr != &mons[PM_ANGEL])
		return((struct monst *)0);
	
	if (MON_AT(x, y)) (void) rloc(m_at(x, y), FALSE);	/* insurance */

	if (!(roamer = makemon(ptr, x, y, NO_MM_FLAGS)))
		return((struct monst *)0);

	EPRI(roamer)->shralign = alignment;
	if (coaligned && !peaceful)
		EPRI(roamer)->renegade = TRUE;
	/* roamer->ispriest == FALSE naturally */
	roamer->isminion = TRUE;	/* borrowing this bit */
	roamer->mtrapseen = ~0;		/* traps are known */
	roamer->mpeaceful = peaceful;
	roamer->msleeping = 0;
	set_malign(roamer); /* peaceful may have changed */

	/* MORE TO COME */
	return(roamer);
}

void
reset_hostility(roamer)
register struct monst *roamer;
{
	if(!(roamer->isminion && (roamer->data == &mons[PM_ALIGNED_PRIEST] ||
				  roamer->data == &mons[PM_ANGEL])))
	        return;

	if(EPRI(roamer)->shralign != u.ualign.type) {
	    roamer->mpeaceful = roamer->mtame = 0;
	    set_malign(roamer);
	}
	newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(mon, x, y)
struct monst *mon;	/* if non-null, <mx,my> overrides <x,y> */
xchar x, y;
{
	register char roomno;
	register struct monst *priest;

	if (mon) {
	    if (is_minion(mon->data) || is_rider(mon->data)) return FALSE;
	    x = mon->mx, y = mon->my;
	}
	if (u.ualign.record <= ALGN_SINNED)	/* sinned or worse */
	    return FALSE;
	if ((roomno = temple_occupied(u.urooms)) == 0 ||
		roomno != *in_rooms(x, y, TEMPLE))
	    return FALSE;
	if ((priest = findpriest(roomno)) == 0)
	    return FALSE;
	return (boolean)(has_shrine(priest) &&
			 p_coaligned(priest) &&
			 priest->mpeaceful);
}

void
ghod_hitsu(priest)	/* when attacking "priest" in his temple */
struct monst *priest;
{
	int x, y, ax, ay, roomno = (int)temple_occupied(u.urooms);
	int x1, y1, x2, y2, n;
	coord poss[4];
	int stpx = sgn(u.ux - priest->mx), stpy = sgn(u.uy - priest->my);
		/* gods avoid hitting the temple priest */
	struct mkroom *troom;

	if (!roomno || !has_shrine(priest))
		return;

	ax = x = EPRI(priest)->shrpos.x;
	ay = y = EPRI(priest)->shrpos.y;
	troom = &rooms[roomno - ROOMOFFSET];

	/*
	 * Determine the source of the lightning bolt according to the
	 * following rules:
	 *	1. The source cannot be directly under the player
	 *	2. Don't zap through the temple priest
	 *	3. First choice of source is the altar itself
	 *	4. Otherwise use a wall, prefering orthogonal to diagonal paths
	 *	5. Choose randomly from equally preferred sources
	 * Note that if the hero is not standing on either the altar or
	 * a door then (u.ux, u.uy) may be counted as a possible source which
	 * is later rejected by linedup() letting the hero off the hook.
	 */
	if((u.ux == x && u.uy == y) || !linedup(u.ux, u.uy, x, y) ||
		stpx == sgn(tbx) && stpy == sgn(tby)) {
	    if(IS_DOOR(levl[u.ux][u.uy].typ)) {

		if(u.ux == troom->lx - 1) {
		    if (stpx != sgn(u.ux - troom->hx) || stpy != 0) {
			x = troom->hx;
			y = u.uy;
		    } else {
			/* Diagonal required */
			x1 = u.ux + u.uy - troom->ly;
			y1 = troom->ly;
			x2 = u.ux + troom->hy - u.uy;
			y2 = troom->hy;
			if (x1 > troom->hx && x2 > troom->hx)
			    return;
			else if (x2 > troom->hx || x1 <= troom->hx && !rn2(2)) {
			    x = x1;
			    y = y1;
			} else {
			    x = x2;
			    y = y2;
			}
		    }
		} else if(u.ux == troom->hx + 1) {
		    if (stpx != sgn(u.ux - troom->lx) || stpy != 0) {
			x = troom->lx;
			y = u.uy;
		    } else {
			/* Diagonal required */
			x1 = u.ux - (u.uy - troom->ly);
			y1 = troom->ly;
			x2 = u.ux - (troom->hy - u.uy);
			y2 = troom->hy;
			if (x1 < troom->lx && x2 < troom->lx)
			    return;
			else if (x2 < troom->lx || x1 >= troom->lx && !rn2(2)) {
			    x = x1;
			    y = y1;
			} else {
			    x = x2;
			    y = y2;
			}
		    }
		} else if(u.uy == troom->ly - 1) {
		    if (stpx != 0 || stpy != sgn(u.uy - troom->hy)) {
			x = u.ux;
			y = troom->hy;
		    } else {
			/* Diagonal required */
			x1 = troom->lx;
			y1 = u.uy + u.ux - troom->lx;
			x2 = troom->hx;
			y2 = u.uy + troom->hx - u.ux;
			if (y1 > troom->hy && y2 > troom->hy)
			    return;
			else if (y2 > troom->hy || y1 <= troom->hy && !rn2(2)) {
			    x = x1;
			    y = y1;
			} else {
			    x = x2;
			    y = y2;
			}
		    }
		} else if(u.uy == troom->hy + 1) {
		    if (stpx != 0 || stpy != sgn(u.uy - troom->ly)) {
			x = u.ux;
			y = troom->ly;
		    } else {
			/* Diagonal required */
			x1 = troom->lx;
			y1 = u.uy - (u.ux - troom->lx);
			x2 = troom->hx;
			y2 = u.uy - (troom->hx - u.ux);
			if (y1 < troom->ly && y2 < troom->ly)
			    return;
			else if (y2 < troom->ly || y1 >= troom->ly && !rn2(2)) {
			    x = x1;
			    y = y1;
			} else {
			    x = x2;
			    y = y2;
			}
		    }
		}
	    } else {
		/* Calculate the possible orthogonal paths */
		n = 0;
		if (stpx != 0 || stpy != sgn(u.uy - troom->ly)) {
		    poss[n].x = u.ux;
		    poss[n++].y = troom->ly;
		}
		if (stpx != 0 || stpy != sgn(u.uy - troom->hy)) {
		    poss[n].x = u.ux;
		    poss[n++].y = troom->hy;
		}
		if (stpx != sgn(u.ux - troom->lx) || stpy != 0) {
		    poss[n].x = troom->lx;
		    poss[n++].y = u.uy;
		}
		if (stpx != sgn(u.ux - troom->hx) || stpy != 0) {
		    poss[n].x = troom->hx;
		    poss[n++].y = u.uy;
		}
		if (n) {
		    n = rn2(n);
		    x = poss[n].x;
		    y = poss[n].y;
		} else {
		    impossible("不存在的祭司？？");
		    return;
		}
	    }
	    if(!linedup(u.ux, u.uy, x, y))
		return;
	}

	switch(rn2(3)) {
	case 0:
	    pline("%s的声音中带着极度的愤怒：“汝将为汝之行为受到无尽折磨！”",
			a_gname_at(ax, ay));
	    break;
	case 1:
	    pline("%s的声音如炸雷一般振响：“汝怎敢伤害吾的仆人？！”",
			s_suffix(a_gname_at(ax, ay)));
	    break;
	default:
	    pline("%s的声音如惊涛骇浪一般炸响：“汝竟敢亵渎吾的圣地！”",
			a_gname_at(ax, ay));
	    break;
	}

	buzz(-10-(AD_ELEC-1), 6, x, y, sgn(tbx), sgn(tby)); /* bolt of lightning */
	exercise(A_WIS, FALSE);
}

void
angry_priest()
{
	register struct monst *priest;
	struct rm *lev;

	if ((priest = findpriest(temple_occupied(u.urooms))) != 0) {
	    wakeup(priest);
	    /*
	     * If the altar has been destroyed or converted, let the
	     * priest run loose.
	     * (When it's just a conversion and there happens to be
	     *	a fresh corpse nearby, the priest ought to have an
	     *	opportunity to try converting it back; maybe someday...)
	     */
	    lev = &levl[EPRI(priest)->shrpos.x][EPRI(priest)->shrpos.y];
	    if (!IS_ALTAR(lev->typ) ||
		((aligntyp)Amask2align(lev->altarmask & AM_MASK) !=
			EPRI(priest)->shralign)) {
		priest->ispriest = 0;		/* now a roamer */
		priest->isminion = 1;		/* but still aligned */
		/* this overloads the `shroom' field, which is now clobbered */
		EPRI(priest)->renegade = 0;
	    }
	}
}

/*
 * When saving bones, find priests that aren't on their shrine level,
 * and remove them.   This avoids big problems when restoring bones.
 */
void
clearpriests()
{
    register struct monst *mtmp, *mtmp2;

    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	mtmp2 = mtmp->nmon;
	if (!DEADMONSTER(mtmp) && mtmp->ispriest && !on_level(&(EPRI(mtmp)->shrlevel), &u.uz))
	    mongone(mtmp);
    }
}

/* munge priest-specific structure when restoring -dlc */
void
restpriest(mtmp, ghostly)
register struct monst *mtmp;
boolean ghostly;
{
    if(u.uz.dlevel) {
	if (ghostly)
	    assign_level(&(EPRI(mtmp)->shrlevel), &u.uz);
    }
}

#endif /* OVLB */

/*priest.c*/
