/*	SCCS Id: @(#)write.c	3.4	2001/11/29	*/
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int FDECL(cost,(struct obj *));

/*
 * returns basecost of a scroll or a spellbook
 */
STATIC_OVL int
cost(otmp)
register struct obj *otmp;
{

	if (otmp->oclass == SPBOOK_CLASS)
		return(10 * objects[otmp->otyp].oc_level);

	/* KMH, balance patch -- restoration of marker charges */
	switch (otmp->otyp) {
# ifdef MAIL
	case SCR_MAIL:
		return(2);
/*		break; */
# endif
	case SCR_LIGHT:
	case SCR_GOLD_DETECTION:
	case SCR_FOOD_DETECTION:
	case SCR_MAGIC_MAPPING:
	case SCR_AMNESIA:
	case SCR_FIRE:
	case SCR_EARTH:
		return(8);
/*		break; */
	case SCR_DESTROY_ARMOR:
	case SCR_CREATE_MONSTER:
	case SCR_PUNISHMENT:
		return(10);
/*		break; */
	case SCR_CONFUSE_MONSTER:
		return(12);
/*		break; */
	case SCR_IDENTIFY:
	case SCR_SCARE_MONSTER:
		return(14);
/*		break; */
	case SCR_TAMING:
	case SCR_TELEPORTATION:
		return(20);
/*		break; */
	/* KMH, balance patch -- more useful scrolls cost more */
	case SCR_STINKING_CLOUD:
	case SCR_ENCHANT_ARMOR:
	case SCR_REMOVE_CURSE:
	case SCR_ENCHANT_WEAPON:
	case SCR_CHARGING:
		return(24);
/*		break; */
	case SCR_GENOCIDE:
		return(30);
/*		break; */
	case SCR_BLANK_PAPER:
	default:
		impossible("你不能写这种奇怪的玩意！");
	}
	return(1000);
}

static NEARDATA const char write_on[] = { SCROLL_CLASS, SPBOOK_CLASS, 0 };

int
dowrite(pen)
register struct obj *pen;
{
	register struct obj *paper;
	char namebuf[BUFSZ], *nm, *bp;
	register struct obj *new_obj;
	int basecost, actualcost;
	int curseval;
	char qbuf[QBUFSZ];
	int first, last, i;
	boolean by_descr = FALSE;
	const char *typeword;

	if (nohands(youmonst.data)) {
	    You("需要手才能写东西！");
	    return 0;
	} else if (Glib) {
	    pline("%s了你的%s。",
		  Tobjnam(pen, "滑出"), makeplural(body_part(FINGER)));
	    dropx(pen);
	    return 1;
	}

	/* get paper to write on */
	paper = getobj(write_on,"可以用魔笔书写的");
	if(!paper)
		return(0);
	typeword = (paper->oclass == SPBOOK_CLASS) ? "法术书" : "scroll";
	if(Blind && !paper->dknown) {
		You("并不知道%s上到底写没写东西！", typeword);
		return(1);
	}
	paper->dknown = 1;
	if(paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
		pline("这个%s不是空的！", typeword);
		exercise(A_WIS, FALSE);
		return(1);
	}

	/* what to write */
	Sprintf(qbuf, "你想写什么%s？", typeword);
	getlin(qbuf, namebuf);
	(void)mungspaces(namebuf);	/* remove any excess whitespace */
	if(namebuf[0] == '\033' || !namebuf[0])
		return(1);
	nm = namebuf;
	if (!strncmpi(nm, "卷轴", 7)) nm += 7;
	else if (!strncmpi(nm, "spellbook ", 10)) nm += 10;
	if (!strncmpi(nm, "", 3)) nm += 3;

	if ((bp = strstri(nm, "防具抚摸")) != 0) {
		(void)strncpy(bp, "防具附魔", 7);	/* won't add '\0' */
		(void)mungspaces(bp + 1);	/* remove the extra space */
	}

	first = bases[(int)paper->oclass];
	last = bases[(int)paper->oclass + 1] - 1;
	for (i = first; i <= last; i++) {
		/* extra shufflable descr not representing a real object */
		if (!OBJ_NAME(objects[i])) continue;

		if (!strcmpi(OBJ_NAME(objects[i]), nm))
			goto found;
		if (!strcmpi(OBJ_DESCR(objects[i]), nm)) {
			by_descr = TRUE;
			goto found;
		}
	}

	There("没有像这样的%s！", typeword);
	return 1;
found:

	if (i == SCR_BLANK_PAPER || i == SPE_BLANK_PAPER) {
		You_cant("写那种卷轴！");
		pline("你特么扯犊子呢你！");
		return 1;
	} else if (i == SPE_BOOK_OF_THE_DEAD) {
		pline("任何地牢冒险家都写不出来那本书。");
		return 1;
	} else if (by_descr && paper->oclass == SPBOOK_CLASS &&
		    !objects[i].oc_name_known) {
		/* can't write unknown spellbooks by description */
		pline(
		  "不幸的是你光知道描述是写不出来对应法术书的。");
		return 1;
	}

	/* KMH, conduct */
	u.uconduct.literate++;

	new_obj = mksobj(i, FALSE, FALSE);
	new_obj->bknown = (paper->bknown && pen->bknown);
#ifdef INVISIBLE_OBJECTS
	new_obj->oinvis = paper->oinvis;
#endif

	/* shk imposes a flat rate per use, not based on actual charges used */
	check_unpaid(pen);

	/* see if there's enough ink */
	basecost = cost(new_obj);
	if(pen->spe < basecost/2)  {
		Your("魔笔剩下的墨不够写那个卷轴！");
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* we're really going to write now, so calculate cost
	 */
	actualcost = rn1(basecost/2,basecost/2);
	curseval = bcsign(pen) + bcsign(paper);
	exercise(A_WIS, TRUE);
	/* dry out marker */
	if (pen->spe < actualcost) {
		pen->spe = 0;
		Your("魔笔彻底用完墨水了！");
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS) {
			pline_The(
		       "法术书没写完，随后上面的所有字迹直接消失了。");
			update_inventory();	/* pen charges */
		} else {
			pline_The("卷轴现在一点用也没有了，然后直接消失了！");
			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}
	pen->spe -= actualcost;

	/* can't write if we don't know it - unless we're lucky */
	if(!(objects[new_obj->otyp].oc_name_known) &&
	   !(objects[new_obj->otyp].oc_uname) &&
	   (rnl(Role_if(PM_WIZARD) ? 3 : 15))) {
		You("%s那个卷轴！", by_descr ? "没写成" : "不知道怎么写出");
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS) {
			You(
       "用你最好的笔迹整了四个大字：“我的日记”。但是字迹马上就消失了。");
			update_inventory();	/* pen charges */
		} else {
			if (by_descr) {
			    Strcpy(namebuf, OBJ_DESCR(objects[new_obj->otyp]));
			    wipeout_text(namebuf, (6+MAXULEV - u.ulevel)/6, 0);
			} else
			    Sprintf(namebuf, "%s到此一游！", plname);
			You("写了“%s”，然后卷轴就凭空消失了。", namebuf);
			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* useup old scroll / spellbook */
	useup(paper);

	/* success */
	if (new_obj->oclass == SPBOOK_CLASS) {
		/* acknowledge the change in the object's description... */
		pline_The("法术书奇怪地扭曲了起来，然后变成了%s样。",
		      OBJ_DESCR(objects[new_obj->otyp]));
	}
	new_obj->blessed = (curseval > 0);
	new_obj->cursed = (curseval < 0);
#ifdef MAIL
	if (new_obj->otyp == SCR_MAIL) new_obj->spe = 1;
#endif
	new_obj = hold_another_object(new_obj, "哎呀！你不小心把%s丢掉了！",
					       The(aobjnam(new_obj, "滑出")),
					       (const char *)0);
	return(1);
}

/*write.c*/
