/*	SCCS Id: @(#)fountain.c	3.4	2003/03/23	*/
/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

STATIC_DCL void NDECL(dowatersnakes);
STATIC_DCL void NDECL(dowaterdemon);
STATIC_DCL void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int,int,genericptr_t));
STATIC_DCL void NDECL(dofindgem);

void
floating_above(what)
const char *what;
{
    You("正高高地飞在%s上面。", what);
}

STATIC_OVL void
dowatersnakes() /* Fountain of snakes! */
{
    register int num = rn1(5,2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
	if (!Blind)
	    pline("一股络绎不绝的%s钻了出来！",
		  Hallucination ? makeplural(rndmonnam()) : "蛇");
	else
	    You_hear("%s嘶嘶叫！", something);
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_WATER_MOCCASIN],
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
    } else
	pline_The("喷泉突然剧烈地冒泡，但随后就平静了下来。");
}

STATIC_OVL
void
dowaterdemon() /* Water demon */
{
    register struct monst *mtmp;

    if(!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
		You("将%s从喷泉中解放了出来！", a_monnam(mtmp));
	    else
		You_feel("某种邪恶的生物现身了。");
/* ------------===========STEPHEN WHITE'S NEW CODE============------------ */
	/* Give those on low levels a (slightly) better chance of survival */
	/* 35% at level 1, 30% at level 2, 25% at level 3, etc... */            
	if (rnd(100) > (60 + 5*level_difficulty())) {
		pline("为了感谢你把%s放了出来，%s决定实现你一个愿望！",
		      mhis(mtmp), mhe(mtmp));
		makewish();
		mongone(mtmp);
	    } else if (t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
	}
    } else
	pline_The("喷泉突然剧烈地冒泡，但随后就平静了下来。");
}

STATIC_OVL void
dowaternymph() /* Water Nymph */
{
	register struct monst *mtmp;

	if(!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
	   (mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy, NO_MM_FLAGS))) {
		if (!Blind)
		   You("吸引了%s！", a_monnam(mtmp));
		else
		   You_hear("诱人的声音。");
		mtmp->msleeping = 0;
		if (t_at(mtmp->mx, mtmp->my))
		    (void) mintrap(mtmp);
	} else
		if (!Blind)
		   pline("一个大泡泡浮到了水面，然后泡泡破了。");
		else
		   You_hear("一声泡泡破掉的声音。");
}

void
dogushforth(drinking) /* Gushing forth along LOS from (u.ux, u.uy) */
int drinking;
{
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t)&madepool);
	if (!madepool) {
	    if (drinking)
		Your("口渴稍微缓和了一点。");
	    else
		pline("你搞了一身水。");
	}
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
	register struct monst *mtmp;
	register struct trap *ttmp;

	if (((x+y)%2) || (x == u.ux && y == u.uy) ||
	    (rn2(1 + distmin(u.ux, u.uy, x, y)))  ||
	    (levl[x][y].typ != ROOM) ||
	    (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
		return;

	if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
		return;

	if (!((*(int *)poolcnt)++))
	    pline("水从满溢的喷泉中爆射而出！");

	/* Put a pool at x, y */
	levl[x][y].typ = POOL;
	/* No kelp! */
	del_engr_at(x, y);
	water_damage(level.objects[x][y], FALSE, TRUE);

	if ((mtmp = m_at(x, y)) != 0)
		(void) minliquid(mtmp);
	else
		newsym(x,y);
}

STATIC_OVL void
dofindgem() /* Find a gem in the sparkling waters. */
{
	if (!Blind) You("看见波光粼粼的水面下有一颗宝石！");
	else You_feel("这里面有个宝石！");
	(void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1),
			 u.ux, u.uy, FALSE, FALSE);
	SET_FOUNTAIN_LOOTED(u.ux,u.uy);
	newsym(u.ux, u.uy);
	exercise(A_WIS, TRUE);			/* a discovery! */
}

void
dryup(x, y, isyou)
xchar x, y;
boolean isyou;
{
	if (IS_FOUNTAIN(levl[x][y].typ) &&
	    (!rn2(3) || FOUNTAIN_IS_WARNED(x,y))) {
		if(isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x,y)) {
			struct monst *mtmp;
			SET_FOUNTAIN_WARNED(x,y);
			/* Warn about future fountain use. */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
			    if (DEADMONSTER(mtmp)) continue;
			    if ((mtmp->data == &mons[PM_WATCHMAN] ||
				mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			       couldsee(mtmp->mx, mtmp->my) &&
			       mtmp->mpeaceful) {
				pline("%s大叫道：", Amonnam(mtmp));
				verbalize("喂！别再使用那个喷泉了！");
				break;
			    }
			}
			/* You can see or hear this effect */
			if(!mtmp) pline_The("喷泉水流变得很细。");
			return;
		}
#ifdef WIZARD
		if (isyou && wizard) {
			if (yn("想要强行让喷泉干涸么？") == 'n')
				return;
		}
#endif
		/* replace the fountain with ordinary floor */
		levl[x][y].typ = ROOM;
		levl[x][y].looted = 0;
		levl[x][y].blessedftn = 0;
		if (cansee(x,y)) pline_The("喷泉干涸了！");
		/* The location is seen if the hero/monster is invisible */
		/* or felt if the hero is blind.			 */
		newsym(x, y);
		level.flags.nfountains--;
		if(isyou && in_town(x, y))
		    (void) angry_guards(FALSE);
	}
}

void
drinkfountain()
{
	/* What happens when you drink from a fountain? */
	register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
	register int fate = rnd(30);

	if (Levitation) {
		floating_above("喷泉");
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

		pline("哇哦！这让你感觉很好！");
		/* blessed restore ability */
		for (ii = 0; ii < A_MAX; ii++)
		    if (ABASE(ii) < AMAX(ii)) {
			ABASE(ii) = AMAX(ii);
			flags.botl = 1;
		    }
		/* gain ability, blessed if "" luck is high */
		i = rn2(A_MAX);		/* start at a random attribute */
		for (ii = 0; ii < A_MAX; ii++) {
		    if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
			break;
		    if (++i >= A_MAX) i = 0;
		}
		display_nhwindow(WIN_MESSAGE, FALSE);
		pline("一股蒸汽从喷泉中逸出……");
		exercise(A_WIS, TRUE);
		levl[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
		pline_The("来上这么一口真是清凉解渴。");
		u.uhunger += rnd(10); /* don't choke on water */
		newuhs(FALSE);
		if(mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			You_feel("对自己有了更多的了解……");
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			exercise(A_WIS, TRUE);
			pline_The("这种感觉消失了。");
			break;

		case 20: /* Foul water */

			pline_The("这水喝起来跟臭了一样！你弯腰然后吐了一地。");
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

			pline_The("水被污染了！");
			if (Poison_resistance) {
			   pline(
			      "说不定这个水是从附近%s生产地流过来的。",
				 fruitname(FALSE));
			   losehp(rnd(4),"没放冰箱还不知道过了多少夜的果汁",
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),"被污染的水", KILLED_BY);
			exercise(A_CON, FALSE);
			break;

		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item */ {
			register struct obj *obj;

			pline("这水不对劲！");
			morehungry(rn1(20, 11));
			exercise(A_CON, FALSE);
			for(obj = invent; obj ; obj = obj->nobj)
				if (!rn2(5))	curse(obj);
			break;
			}

		case 25: /* See invisible */

			if (Blind) {
			    if (Invisible) {
				You("感觉变透明了。");
			    } else {
			    	You("感觉对自己非常了解。");
			    	pline("然后这种感觉就消退了。");
			    }
			} else {
			   You("似乎看到有个人影逐渐接近你。");
			   pline("但这种感觉很快就消失了。");
			}
			HSee_invisible |= FROMOUTSIDE;
			newsym(u.ux,u.uy);
			exercise(A_WIS, TRUE);
			break;

		case 26: /* See Monsters */

			(void) monster_detect((struct obj *)0, 0);
			exercise(A_WIS, TRUE);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			register struct monst *mtmp;

			pline("这个水搞得你口气很臭！");
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			    if(!DEADMONSTER(mtmp))
				monflee(mtmp, 0, FALSE, FALSE);
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(TRUE);
			break;

		default:

			pline("这个水尝起来什么味也没有。");
			break;
	    }
	}
	dryup(u.ux, u.uy, TRUE);
}

void
dipfountain(obj)
register struct obj *obj;
{
	if (Levitation) {
		floating_above("喷泉");
		return;
	}

	/* Don't grant Excalibur when there's more than one object.  */
	/* (quantity could be > 1 if merged daggers got polymorphed) */

	if (obj->otyp == LONG_SWORD && obj->quan == 1L
	    && u.ulevel > 4 && !rn2(8) && !obj->oartifact
	    && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {

		if (u.ualign.type != A_LAWFUL) {
			/* Ha!  Trying to cheat her. */
			pline("一股寒冷刺骨的雾从喷泉中升起，随后包裹住了你的长剑。");
			pline_The("喷泉消失了！");
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = FALSE;
			exercise(A_WIS, FALSE);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
	  pline("一只手从喷泉深处伸了出来，然后祝福了那把长剑。");
			pline("然后那只手缩了回去，随着消失的还有这座喷泉！");
			obj = oname(obj, artiname(ART_EXCALIBUR));
			discover_artifact(ART_EXCALIBUR);
			bless(obj);
			obj->oeroded = obj->oeroded2 = 0;
			obj->oerodeproof = TRUE;
			exercise(A_WIS, TRUE);
		}
		update_inventory();
		levl[u.ux][u.uy].typ = ROOM;
		levl[u.ux][u.uy].looted = 0;
		newsym(u.ux, u.uy);
		level.flags.nfountains--;
		if(in_town(u.ux, u.uy))
		    (void) angry_guards(FALSE);
		return;
	} else if (get_wet(obj, FALSE) && !rn2(2))
		return;

	/* Acid and water don't mix */
	if (obj->otyp == POT_ACID) {
	    useup(obj);
	    return;
	}

	switch (rnd(30)) {
		case 10: /* Curse the item */
			curse(obj);
			break;
		case 11:
		case 12:
		case 13:
		case 14: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline_The("水面发光了片刻。");
			    uncurse(obj);
			} else {
			    pline("你突然有一股失落感。");
			}
			break;
		case 15:
		case 16: /* Water Demon */
			dowaterdemon();
			break;
		case 17:
		case 18: /* Water Nymph */
			dowaternymph();
			break;
		case 19:
		case 20: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 21:
		case 22:
		case 23: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 24:
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
			pline("你的%s有一股奇怪的刺痛感。",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel("浑身突然一颤。");
			break;
		case 28: /* Strange feeling */
			pline("你突然特别想洗个澡。");
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You("在喷泉里丢了一些身上的金币！");
			    CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			    exercise(A_WIS, FALSE);
			}
#else
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10; 
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;				  
				    if (!otmp->quan) delobj(otmp);
				}
			        You("在喷泉里丢了一些身上的金币！");
				CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			        exercise(A_WIS, FALSE);
                            }
			}
#endif
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (FOUNTAIN_IS_LOOTED(u.ux,u.uy)) break;
		    SET_FOUNTAIN_LOOTED(u.ux,u.uy);
		    (void) mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
		pline("你看到水底有一些金币闪着金光。");
		    exercise(A_WIS, TRUE);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, TRUE);
}

#ifdef SINKS
void
diptoilet(obj)
register struct obj *obj;
{
	if (Levitation) {
	    floating_above("马桶");
	    return;
	}
	(void) get_wet(obj, FALSE);
	/* KMH -- acid and water don't mix */
	if (obj->otyp == POT_ACID) {
	    useup(obj);
	    return;
	}
	if(is_poisonable(obj)) {
	    if (flags.verbose)  You("把它用粑粑糊了一圈。");
	    obj->opoisoned = TRUE;
	}
	if (obj->oclass == FOOD_CLASS) {
	    if (flags.verbose)  pline("哦呦！它看起来好像更好吃了耶……");
	    obj->orotten = TRUE;
	}
	if (flags.verbose)  pline("真恶心啊！");
}


void
breaksink(x,y)
int x, y;
{
    if(cansee(x,y) || (x == u.ux && y == u.uy))
	pline_The("水管破了！水喷的到处都是！");
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
}

void
breaktoilet(x,y)
int x, y;
{
    register int num = rn1(5,2);
    struct monst *mtmp;
    pline("马桶突然碎掉了！");
    level.flags.nsinks--;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
    if (!rn2(3)) {
      if (!(mvitals[PM_BABY_CROCODILE].mvflags & G_GONE)) {
	if (!Blind) {
	    if (!Hallucination) pline("坏了！水蝮蛇从管道里钻了出来！");
	    else pline("糟了！好多的罂粟花丛这里头钻了出来！");
	} else
	    You("听见周围有什么东西正在你身边滑动！");
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_BABY_CROCODILE],u.ux,u.uy, NO_MM_FLAGS)) &&
	       t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
      } else
	pline("下水道静的诡异。");
    }
}

void
drinksink()
{
	struct obj *otmp;
	struct monst *mtmp;

	if (Levitation) {
		floating_above("水槽");
		return;
	}
	switch(rn2(20)) {
		case 0: You("喝了一口很冷的水。");
			break;
		case 1: You("喝了一口很温热的水。");
			break;
		case 2: You("喝了一口特别烫的水。");
			if (Fire_resistance)
				pline("它看起来似乎挺美味。");
			else losehp(rnd(6), "喝开水", KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The("水槽看起来很脏。");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				if (mtmp) pline("娘嘞！水槽里头有只%s！",
					(Blind || !canspotmon(mtmp)) ?
					"蠕动的生物" :
					a_monnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(POTION_CLASS,FALSE);
				if (otmp->otyp == POT_WATER) {
					obfree(otmp, (struct obj *)0);
					otmp = (struct obj *) 0;
				}
			} while(!otmp);
			otmp->cursed = otmp->blessed = 0;
			pline("有些%s色的液体从水龙头中流出。",
			      Blind ? "奇怪的" :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));
			otmp->dknown = !(Blind || Hallucination);
			otmp->fromsink = 1; /* kludge for docall() */
			/* dopotion() deallocs dummy potions */
			(void) dopotion(otmp);
			break;
		case 5: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You("在水槽中找到了一个戒指！");
			    (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
			} else pline("有些脏水从下水道里返了上来。");
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
		case 7: pline_The("水开始按照它的意愿移动！");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline("但水面很快就安定下来了。");
			break;
		case 8: pline("哕，这水尝起来太恶心了。");
			more_experienced(1,0);
			newexplevel();
			break;
		case 9: pline("呕呕呕呕……这水喝起来像下水道反上来的！你疯狂呕吐。");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
		case 10:
			/* KMH, balance patch -- new intrinsic */
			pline("这个水里头有剧毒核废料！");
			if (!Unchanging) {
			if (!Unchanging) {
				You("经历了一场特别可怕的变形！");
				polyself(FALSE);
			}
			}
			break;
		/* more odd messages --JJB */
		case 11: You_hear("水管中传来敲打声……");
			break;
		case 12: You_hear("下水道水管中传来一阵歌声……");
			break;
		case 19: if (Hallucination) {
		   pline("一只手从污了吧唧的排水管中深处……--你在期待什么？--");
				break;
			}
		default: You("喝了一口%s水。",
			rn2(3) ? (rn2(2) ? "冷" : "温暖的") : "滚烫的");
	}
}

void
drinktoilet()
{
	if (Levitation) {
		floating_above("马桶");
		return;
	}
	if ((youmonst.data->mlet == S_DOG) && (rn2(5))){
		pline("这马桶里头的水还挺爽口的！");
		u.uhunger += 10;
		return;
	}
	switch(rn2(9)) {
/*
		static NEARDATA struct obj *otmp;
 */
		case 0: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline("这个马桶看起来有点脏。");
			else {
				static NEARDATA struct monst *mtmp;

				mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy,
					NO_MM_FLAGS);
				pline("妈耶！马桶里头有只%s！",
					Blind ? "蠕动的生物" :
					a_monnam(mtmp));
			}
			break;
		case 1: breaktoilet(u.ux,u.uy);
			break;
		case 2: pline("有什么东西从马桶里钻了出来！");
			if (mvitals[PM_BROWN_PUDDING].mvflags & G_GONE
			    || !makemon(&mons[PM_BROWN_PUDDING], u.ux, u.uy,
					NO_MM_FLAGS))
				pline("但它又滑了回去。");
			break;
		case 3:
		case 4: if (mvitals[PM_BABY_CROCODILE].mvflags & G_GONE)
				pline("马桶闻起来有点腥臊。");
			else {
				static NEARDATA struct monst *mtmp;

				mtmp = makemon(&mons[PM_BABY_CROCODILE], u.ux,
					 u.uy, NO_MM_FLAGS);
				pline("我了个大去！马桶里头有只%s！",
					Blind ? "蠕动的生物" :
					a_monnam(mtmp));
			}
			break;
		default: pline("呕呕呕呕……这水喝起来像下水道反上来的！你疯狂呕吐。");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
	}
}
#endif /* SINKS */


void
whetstone_fountain_effects(obj)
register struct obj *obj;
{
	if (Levitation) {
		floating_above("喷泉");
		return;
	}

	switch (rnd(30)) {
		case 10: /* Curse the item */
			curse(obj);
			break;
		case 11:
		case 12:
		case 13:
		case 14: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline_The("水面发光了片刻。");
			    uncurse(obj);
			} else {
			    pline("你突然有一股失落感。");
			}
			break;
		case 15:
		case 16: /* Water Demon */
			dowaterdemon();
			break;
		case 17:
		case 18: /* Water Nymph */
			dowaternymph();
			break;
		case 19:
		case 20: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 21:
		case 22:
		case 23: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 24:
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
			pline("你的%s有一股奇怪的刺痛感。",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel("浑身突然一颤。");
			break;
		case 28: /* Strange feeling */
			pline("你突然特别想洗个澡。");
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You("在喷泉里丢了一些身上的金币！");
			    CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			    exercise(A_WIS, FALSE);
			}
#else
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10; 
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;				  
				    if (!otmp->quan) delobj(otmp);
				}
			        You("在喷泉里丢了一些身上的金币！");
			        levl[u.ux][u.uy].looted &= ~F_LOOTED;
			        exercise(A_WIS, FALSE);
                            }
			}
#endif
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (levl[u.ux][u.uy].looted) break;
		    levl[u.ux][u.uy].looted |= F_LOOTED;
		    (void) mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
		pline("你看到水底有一些金币闪着金光。");
		    exercise(A_WIS, TRUE);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, TRUE);
}

#ifdef SINKS

void
whetstone_toilet_effects(obj)
register struct obj *obj;
{
	if (Levitation) {
	    floating_above("马桶");
	    return;
	}
	if(is_poisonable(obj)) {
	    if (flags.verbose)  You("把它用粑粑糊了一圈。");
	    obj->opoisoned = TRUE;
	}
	if (flags.verbose)  pline("真恶心啊！");
}

void
whetstone_sink_effects(obj)
register struct obj *obj;
{
	struct monst *mtmp;

	if (Levitation) {
		floating_above("水槽");
		return;
	}
	switch(rn2(20)) {
		case 0: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The("水槽看起来很脏。");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				pline("娘嘞！水槽里头有只%s！",
					Blind ? "蠕动的生物" :
					a_monnam(mtmp));
			}
			break;
		case 1: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You("在水槽中找到了一个戒指！");
			    (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
			} else pline("有些脏水从下水道里返了上来。");
			break;
		case 2: breaksink(u.ux,u.uy);
			break;
		case 3: pline_The("水开始按照它的意愿移动！");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline("但水面很快就安定下来了。");
			break;
		case 4:
			pline("这个水里头有剧毒核废料！");
			obj = poly_obj(obj, STRANGE_OBJECT);
			u.uconduct.polypiles++;
			break;
		case 5: You_hear("水管中传来敲打声……");
			break;
		case 6: You_hear("下水道水管中传来一阵歌声……");
			break;
		case 19: if (Hallucination) {
		   pline("一只手从污了吧唧的排水管中深处……--你在期待什么？--");
				break;
			}
		default:
			break;
	}
}

#endif /* SINKS */

/*fountain.c*/
