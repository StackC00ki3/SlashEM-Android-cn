/*** gypsy.c ***/

#include "hack.h"
#include "egyp.h"
#include "qtext.h"


/* To do:
 *	fortune_lev()
 *	Fourtunes for suited cards
 *	On-line help
 */


/*** Money-related functions ***/

static void
gypsy_charge (mtmp, amount)
	struct monst *mtmp;
	long amount;
{
#ifdef GOLDOBJ
	struct obj *gypgold;
#endif

	/* Take from credit first */
	if (amount > EGYP(mtmp)->credit) {
		/* Do in several steps, for broken compilers */
		amount -= EGYP(mtmp)->credit;
		EGYP(mtmp)->credit = 0;
#ifdef GOLDOBJ
		money2mon(mtmp, amount);
#else
		u.ugold -= amount;
#endif
		flags.botl = 1;
	} else
		EGYP(mtmp)->credit -= amount;

	/* The gypsy never carries cash; it might get stolen! */
#ifdef GOLDOBJ
	gypgold = findgold(mtmp->minvent);
	if (gypgold)
		m_useup(mtmp, gypgold);
#endif
	return;
}

static boolean
gypsy_offer (mtmp, cost, txt)
	struct monst *mtmp;
	long cost;
	char *txt;
{
#ifdef GOLDOBJ
	long umoney;
	umoney = money_cnt(invent);
#endif
	verbalize("如果你愿意支付给我%s的信用额度，我就%s！", cost, txt);
	if (EGYP(mtmp)->credit >= cost) {
		if (yn("接受吉普赛人的许愿机会？") == 'y') {
			EGYP(mtmp)->credit -= cost;
			return (TRUE);
		}
#ifndef GOLDOBJ
	} else if (EGYP(mtmp)->credit + u.ugold >= cost)
		verbalize("唉，我不收黄金，真可惜啊！");
#else
	} else if (EGYP(mtmp)->credit + umoney >= cost)
		verbalize("唉，我不收现金，真可惜啊！");
#endif
		/* Maybe you could try gambling some of it for credit... */
	else
		verbalize("唉，你记的信用帐不够，真可惜啊！");
	return (FALSE);
}

static long
gypsy_bet (mtmp, minimum)
	struct monst *mtmp;
	long minimum;
{
	char prompt[BUFSZ], buf[BUFSZ];
	long bet = 0L;
#ifdef GOLDOBJ
	long umoney;
	umoney = money_cnt(invent);
#endif

	if (minimum > EGYP(mtmp)->credit + 
#ifndef GOLDOBJ
													u.ugold) {
#else
 													umoney) {		
#endif
		You("身上甚至没有付最低赌注的钱。");
		return (0L);
	}

	/* Prompt for an amount */
	Sprintf(prompt, "你想赌多少钱？（下注范围：%ld到%ld）", minimum,
			EGYP(mtmp)->credit + 
#ifndef GOLDOBJ
													u.ugold);
#else
													umoney);													
#endif
	getlin(prompt, buf);
	(void) sscanf(buf, "%ld", &bet);

	/* Validate the amount */
	if (bet == 0L) {
		pline("算了。");
		return (0L);
	}
	if (bet < minimum) {
		You("至少要下注%ld。", minimum);
		return (0L);
	}
	if (bet > EGYP(mtmp)->credit +
#ifndef GOLDOBJ
								u.ugold) {
#else
								umoney) {												
#endif
		You("拿不出这么多钱当赌注！");
		return (0L);
	}
	return (bet);
}


/*** Card-related functions ***/

static const char *suits[CARD_SUITS] =
{ "宝剑", "魔杖",     "盾牌",  "戒指" };          /* Special */
/* swords    wands/rods  roses/cups  pentacles/disks/coins  Tarot */
/* spade     bastoni     coppe       denari                 Italian */
/* swords    batons      cups        coins                  (translated) */
/* spades    clubs       hearts      diamonds               French */


static const char *ranks[CARD_RANKS] =
{ "A", "2", "3", "4", "5", "6", "7", "8", "9", "10",
   /*none*/       "骑士",       "皇后", "国王" }; /* French */
/* page/princess  knight/prince  queen    king        Tarot */


static const char *trumps[CARD_TRUMPS] =
{	"愚者",               /* This is NOT a Joker */
	"魔术师",           /* same as the Magus */
	"女祭司",     /* sometimes placed after the Emperor */
#if 0
	"女皇",            /* not included here */
	"皇帝",            /* not included here */
#endif
	"神谕",             /* same as the Hierophant */
	"恋人",
	"战车",
	"力量",               /* sometimes Adjustment */
	"隐者",
	"命运之轮",   /* sometimes Fortune */
	"正义",                /* sometimes Lust */
	"惩罚",             /* replaces the Hanged Man */
	"恶魔",              /* normally #15 */
	"咒术师",                /* replaces Art or Temperance */
	"死神",                  /* swapped with the Devil so it remains #13 */
	"高塔",              /* really! */
	"星星",
	"月亮",
	"太阳",
	"审判",              /* sometimes Aeon */
	"无限"                /* replaces the World or the Universe */
};


static void
card_shuffle (mtmp)
	struct monst *mtmp;
{
	xchar *cards = &EGYP(mtmp)->cards[0];
	int i, j, k;


	pline("%s把塔罗牌打乱后重新洗了一次。", Monnam(mtmp));
	for (i = 0; i < CARD_TOTAL; i++)
		/* Initialize the value */
		cards[i] = i;
	for (i = 0; i < CARD_TOTAL; i++) {
		/* Swap this value with another randomly chosen one */
		j = rn2(CARD_TOTAL);
		k = cards[j];
		cards[j] = cards[i];
		cards[i] = k;
	}
	EGYP(mtmp)->top = CARD_TOTAL;
}

static xchar
card_draw (mtmp)
	struct monst *mtmp;
{
	if (EGYP(mtmp)->top <= 0)
		/* The deck is empty */
		return (-1);
	return (EGYP(mtmp)->cards[--EGYP(mtmp)->top]);
}

static void
card_name (num, buf)
	xchar num;
	char *buf;
{
	int r, s;


	if (!buf) return;
	if (Hallucination) num = rn2(CARD_TOTAL);
	if (num < 0 || num >= CARD_TOTAL) {
		/* Invalid card */
		impossible("没有%d这种牌！", num);
		Strcpy(buf, "");
	} else if (card_istrump(num)) {
		/* Handle trump cards */
		r = card_trump(num);
		if (!r)
			Sprintf(buf, "", trumps[r]);
		else
			Sprintf(buf, "", r, trumps[r]);
	} else {
		/* Handle suited cards */
		r = card_rank(num);
		s = card_suit(num);
		Sprintf(buf, "", ranks[r], suits[s]);
	}
	return;
}


/*** Fortunes ***/

#define FORTUNE_COST	50			/* Cost to play */

static short birthstones[12] =
{
	/* Jan */  GARNET,      /* Feb */  AMETHYST,
	/* Mar */  AQUAMARINE,  /* Apr */  DIAMOND,
	/* May */  EMERALD,     /* Jun */  OPAL,
	/* Jul */  RUBY,        /* Aug */  CHRYSOBERYL,
	/* Sep */  SAPPHIRE,    /* Oct */  BLACK_OPAL,
	/* Nov */  TOPAZ,       /* Dec */  TURQUOISE
};


static void
fortune_lev (mtmp, name, txt)
	struct monst *mtmp;
	char *name, *txt;
{
	/*** FIXME -- still very buggy ***/
/*	d_level *lev;*/
	schar dep;


	dep = lev_by_name(name);
	if (!dep) {
		/* Perhaps the level doesn't exist? */
		verbalize("你只能看到雾蒙蒙的一片。");
		return;
	}

	if (dep == depth(&u.uz))
		verbalize("我看见%s就在这里。", txt);
	else {
		verbalize("我看见了……%s正待在%d层中。", txt, (int)dep);
/*		if (gypsy_offer(mtmp, 5000L, ""))
			;*/
	}
	return;
}

static void
fortune (mtmp)
	struct monst *mtmp;
{
	xchar card;
	char buf[BUFSZ];
	short otyp;
	struct obj *otmp;


	/* Shuffle the deck, if neccessary, and draw a card */
	gypsy_charge(mtmp, FORTUNE_COST);
	if (EGYP(mtmp)->top <= 0)
		card_shuffle(mtmp);
	card = card_draw(mtmp);
#ifdef WIZARD
	if (wizard) {
		long t = -1;

		getlin("你想直接抽哪张塔罗牌？", buf);
		(void) sscanf(buf, "%ld", &t);
		if (t >= 0) card = t + CARD_SUITED;
	}
#endif
	card_name(card, buf);
	verbalize("你抽出了一张%s。", buf);

	if (card_istrump(card))
		switch (card_trump(card)) {
		case 0:	/* the Fool */
			adjattrib(A_WIS, -1, 0);
			change_luck(-3);
			break;
		case 1:	/* the Magician */
			if (u.uevent.udemigod)
				resurrect();
			else
				fortune_lev(mtmp, "",
					"这是前往巫师塔的一个入口");
				/*fortune_lev(mtmp, &portal_level);*/
			break;
		case 2: /* the High Priestess */
			if (u.uhave.amulet)
				verbalize("我看见……我看见在天堂之上有一个至高祭坛。");
				/* Can only get there by ascending... */
			else
				verbalize("我在%d层看见了一个至高祭坛。",
						depth(&sanctum_level));
				/* Can only get there by invocation... */
			break;
		case 3: /* the Oracle */
			fortune_lev(mtmp, "神谕", "神谕");
			/*fortune_lev(mtmp, &oracle_level);*/
			break;
		case 4: /* the Lovers */
			makemon(&mons[flags.female ? PM_INCUBUS : PM_SUCCUBUS],
				u.ux, u.uy, 0);
			break;
		case 5: /* the Chariot */
			if (gypsy_offer(mtmp, 5000L,
					"把你传送到你想去的楼层")) {
				incr_itimeout(&HTeleport_control, 1);
				level_tele();
			}
			break;
		case 6: /* Strength */
			adjattrib(A_STR, 1, 0);
			incr_itimeout(&HHalf_physical_damage, rn1(500, 500));
			break;
		case 7: /* the Hermit */
			You_feel("感觉自己想要躲躲藏藏！");
			incr_itimeout(&HTeleportation, rn1(300, 300));
			incr_itimeout(&HInvis, rn1(500, 500));
			newsym(u.ux, u.uy);
			break;
		case 8: /* the Wheel of Fortune */
			if (Hallucination)
				pline("维娜跑哪去了？");
			else
				You_feel("很幸运！");
			if (u.uluck < 0)
				u.uluck = 0;
			else
				change_luck(3);
			break;
		case 9: /* Justice */
			makemon(&mons[PM_ERINYS], u.ux, u.uy, 0);
			break;
		case 10: /* Punishment */
			if (!Punished)
				punish((struct obj *)0);
			else
				rndcurse();
			break;
		case 11: /* the Devil */
			summon_minion(A_NONE, TRUE);
			break;
		case 12: /* Sorcery */
			adjattrib(urole.spelstat, 1, 0);
			incr_itimeout(&HHalf_spell_damage, rn1(500, 500));
			break;
		case 13: /* Death */
			if (nonliving(youmonst.data) || is_demon(youmonst.data) 
					|| Antimagic)
				shieldeff(u.ux, u.uy);
			else if(Hallucination)
				You("有一种灵魂出窍的感觉。");
			else  {
				killer_format = KILLED_BY;
				killer = "死神塔罗牌";
				done(DIED);
			}
			break;
		case 14: /* the Tower */
			fortune_lev(mtmp, "", "穿刺公弗拉德");
			/* fortune_lev(mtmp, &vlad_level); */
			break;
		case 15: /* the Star */
			otyp = birthstones[getmonth()];
			makeknown(otyp);
			if ((otmp = mksobj(otyp, TRUE, FALSE)) != (struct obj *)0) {
				pline("%s把手伸到你的%s后面，然后拿出了%s。",
						Monnam(mtmp), body_part(HEAD), doname(otmp));
				if (pickup_object(otmp, otmp->quan, FALSE) <= 0) {
					obj_extract_self(otmp);
					place_object(otmp, u.ux, u.uy);
					newsym(u.ux, u.uy);
				}
			}
			break;
		case 16: /* the Moon */
			/* Reset the old moonphase */
			if (flags.moonphase == FULL_MOON)
				change_luck(-1);

			/* Set the new moonphase */
			flags.moonphase = phase_of_the_moon();
			switch (flags.moonphase) {
				case NEW_MOON:
					pline("小心！今晚新月。");
					break;
				case 1:	case 2:	case 3:
					pline_The("月亮正渐渐升起。");
					break;
				case FULL_MOON:
					You("很幸运！今晚是满月。");
					change_luck(1);
					break;
				case 5:	case 6:	case 7:
					pline_The("今晚月亮是下凸月。");
					break;
				default:
					impossible("哦%d好奇怪啊", flags.moonphase);
					break;
			}
			break;
		case 17: /* the Sun */
			if (midnight())
				verbalize("现在这个点女巫应该会出来逛，总之你要小心亡灵！");
			else if (night())
				verbalize("现在这个点是晚上，小心那些晚上会出来游荡的生物哦！");
			else
				verbalize("这个点是白天啊，你不去工作你搁这玩什么slashem？");
			break;
		case 18: /* Judgement */
			fortune_lev(mtmp, "前往任务位面的传送门",
				"前往任务位面的传送门");
			/* fortune_lev(mtmp, &quest_level); */
			break;
		case 19: /* Infinity */
			if (mtmp->mcan) {
				verbalize("我倒是想许愿我没在这里出现过！");
				mongone(mtmp);
			} else if (gypsy_offer(mtmp, 10000L, "实现你一个愿望")) {
				mtmp->mcan = TRUE;
				makewish();
			}
			break;
		default:
			impossible("未知塔罗牌%d", card_trump(card));
			break;
		}	/* End trumps */
	else
		/* Suited card */
		com_pager(QT_GYPSY + card);

	return;
}


/*** Three-card monte ***/

#define MONTE_COST	1			/* Minimum bet */
#define MONTE_MAX	10			/* Maximum value of monteluck */


static void
monte (mtmp)
	struct monst *mtmp;
{
	long bet, n;
	char buf[BUFSZ];
	winid win;
	anything any;
	menu_item *selected;
	int delta;


	/* Get the bet */
	bet = gypsy_bet(mtmp, MONTE_COST);
	if (!bet) return;

	/* Shuffle and pick */
	if (flags.verbose)
		pline("%s摆出了三张牌，然后把它们洗了一遍。", Monnam(mtmp));
	any.a_void = 0;	/* zero out all bits */
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_char = 'l';
	add_menu(win, NO_GLYPH, &any , 'l', 0, ATR_NONE,
			"左边的扑克牌", MENU_UNSELECTED);
	any.a_char = 'c';
	add_menu(win, NO_GLYPH, &any , 'c', 0, ATR_NONE,
			"中间的扑克牌", MENU_UNSELECTED);
	any.a_char = 'r';
	add_menu(win, NO_GLYPH, &any , 'r', 0, ATR_NONE,
			"右边的扑克牌", MENU_UNSELECTED);
	end_menu(win, "选一张牌：");
	while (select_menu(win, PICK_ONE, &selected) != 1) ;
	destroy_nhwindow(win);

	/* Calculate the change in odds for next time */
	/* Start out easy, but get harder once the player is suckered */
	delta = rnl(4) - 3;	/* Luck helps */
	if (u.umontelast == selected[0].item.a_char)
		/* Only suckers keep picking the same card */
		delta++;
	u.umontelast = selected[0].item.a_char;
	for (n = bet; n > 0; n /= 10L)
		/* Penalize big bets */
		delta++;
/*	pline("", u.umonteluck, delta);*/

	/* Did we win? */
	if (u.umonteluck <= rn2(MONTE_MAX)) {
		if (u.umonteluck == 0)
			verbalize("你赢啦！你看，这游戏简单吧？");
		else
			verbalize("你赢了！");
		EGYP(mtmp)->credit += bet;

		/* Make it harder for next time */
		if (delta > 0) u.umonteluck += delta;
		if (u.umonteluck > MONTE_MAX) u.umonteluck = MONTE_MAX;
	} else {
		card_name(rn1(2, 1), buf);
		verbalize("对不住了，你刚刚抓的牌是%s。再试一次吧。", buf);
		gypsy_charge(mtmp, bet);

		/* Make it a little easier for next time */
		if (delta < 0) u.umonteluck += delta;
		if (u.umonteluck < 0) u.umonteluck = 0;
	}
	return;
}


/*** Ninety-nine ***/

#define NINETYNINE_COST		1	/* Minimum bet */
#define NINETYNINE_HAND		3	/* Number of cards in hand */
#define NINETYNINE_GOAL		99	/* Limit of the total */

static boolean
nn_playable (card, total)
	xchar card;
	int total;
{
	if (card_istrump(card))
		/* The fool always loses; other trumps are always playable */
		return (card != CARD_SUITED);
	switch (card_rank(card)+1) {
		case 11:	/* Jack */
		case 12:	/* Queen */
			return (total >= 10);
		case 13:	/* King */
			return (TRUE);
		default:	/* Ace through 10 */
			return ((total + card_rank(card) + 1) <= NINETYNINE_GOAL);
	}
}

static int
nn_play (card, total)
	xchar card;
	int total;
{
	if (card_istrump(card)) {
		if (card == CARD_SUITED)
			/* The Fool always loses */
			return (NINETYNINE_GOAL+1);
		else
			/* Other trumps leave the total unchanged */
			return (total);
	}
	switch (card_rank(card)+1) {
		case 11:	/* Jack */
		case 12:	/* Queen */
			return (total - 10);
		case 13:	/* King */
			return (NINETYNINE_GOAL);
		default:	/* Ace through 10 */
			return (total + card_rank(card) + 1);
	}
}

static int
nn_pref (card)
	xchar card;
{
	/* Computer's preferences for playing cards:
	 * 3.  Get rid of Ace through 10 whenever we can.  Highest priority.
	 * 2.  King will challenge the player.  High priority.
	 * 1.  Jack and queen may help us, or the hero.  Low priority. 
	 * 0.  Trumps can always be played (except the fool).  Lowest priority.
	 */
	if (card_istrump(card))
		/* The fool always loses; other trumps are always playable */
		return (0);
	switch (card_rank(card)+1) {
		case 11:	/* Jack */
		case 12:	/* Queen */
			return (1);
		case 13:	/* King */
			return (2);
		default:	/* Ace through 10 */
			return (3);
	}
}


static void
ninetynine (mtmp)
	struct monst *mtmp;
{
	long bet;
	int i, n, which, total = 0;
	xchar uhand[NINETYNINE_HAND], ghand[NINETYNINE_HAND];
	char buf[BUFSZ];
	winid win;
	anything any;
	menu_item *selected;


	/* Get the bet */
	bet = gypsy_bet(mtmp, NINETYNINE_COST);
	if (!bet) return;

	/* Shuffle the deck and deal */
	card_shuffle(mtmp);
	for (i = 0; i < NINETYNINE_HAND; i++) {
		uhand[i] = card_draw(mtmp);
		ghand[i] = card_draw(mtmp);
	}

	while (1) {
		/* Let the user pick a card */
		any.a_void = 0;	/* zero out all bits */
		win = create_nhwindow(NHW_MENU);
		start_menu(win);
		for (i = 0; i < NINETYNINE_HAND; i++) {
			any.a_int = (nn_playable(uhand[i], total) ? i+1 : 0);
			card_name(uhand[i], buf);
			add_menu(win, NO_GLYPH, &any , 0, 0, ATR_NONE,
					buf, MENU_UNSELECTED);
		}
		any.a_int = NINETYNINE_HAND + 1;
		add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				"弃权", MENU_UNSELECTED);
		end_menu(win, "打出一张牌：");
		while (select_menu(win, PICK_ONE, &selected) != 1) ;
		destroy_nhwindow(win);

		/* Play the card */
		which = selected[0].item.a_int-1;
		if (which >= NINETYNINE_HAND) {
			You("弃权了。");
			gypsy_charge(mtmp, bet);
			return;
		}
		card_name(uhand[which], buf);
		total = nn_play(uhand[which], total);
		You("打出了%s，当前总计点数为%d。", buf, total);
		if (total < 0 || total > NINETYNINE_GOAL) {
			You("输了！");
			gypsy_charge(mtmp, bet);
			return;
		}

		/* Draw a new card */
		uhand[which] = card_draw(mtmp);
		if (uhand[which] < 0) {
			pline_The("桌上的牌已经打完了，你赢了！");
			EGYP(mtmp)->credit += bet;
			return;
		}

		/* Let the gypsy pick a card */
		n = 0;
		for (i = 0; i < NINETYNINE_HAND; i++)
			if (nn_playable(ghand[i], total)) {
				/* The card is playable, but is it the best? */
				if (!n++ || nn_pref(ghand[i]) > nn_pref(ghand[which]))
					which = i;
			}
		if (!n) {
			/* No playable cards */
			pline("%s因牌库没牌而投降了。你赢了！", Monnam(mtmp));
			EGYP(mtmp)->credit += bet;
			return;
		}

		/* Play the card */
		card_name(ghand[which], buf);
		total = nn_play(ghand[which], total);
		pline("%s打出了一张%s，其当前总计点数为%d。", Monnam(mtmp), buf, total);

		/* Draw a new card */
		ghand[which] = card_draw(mtmp);
		if (ghand[which] < 0) {
			pline_The("桌上的牌已经打完了，你赢了！");
			EGYP(mtmp)->credit += bet;
			return;
		}
	}

	return;
}



/*** Pawn gems ***/

STATIC_OVL NEARDATA const char pawnables[] = { ALLOW_COUNT, GEM_CLASS, 0 };

static void
pawn (mtmp)
	struct monst *mtmp;
{
	struct obj *otmp;
	long value;


	/* Prompt for an item */
	otmp = getobj((const char *)pawnables, "抵押");

	/* Is the item valid? */
	if (!otmp) return;
	if (!objects[otmp->otyp].oc_name_known) {
		/* Reject unknown objects */
		verbalize("你确定你给我的这个宝石有人验过真假吗？");
		return;
	}
	if (otmp->otyp < DILITHIUM_CRYSTAL || otmp->otyp > LAST_GEM) {
		/* Reject glass */
		verbalize("去你丫的，别拿这个垃圾耍我！");
		return;
	}

	/* Give the credit */
	value = otmp->quan * objects[otmp->otyp].oc_cost;
	pline("%s给你算了%ldzorkmid的信用额度。", Monnam(mtmp),
			value, plur(value));
	EGYP(mtmp)->credit += value;

	/* Gypsies don't keep merchandise; it could get stolen! */
	otmp->quan = 1L;
	useup(otmp);
	return;
}


/*** Yendorian Tarocchi ***/

#define TAROCCHI_COST	500		/* Cost to play */
#define TAROCCHI_HAND	10		/* Number of cards in hand */

static void
tarocchi (mtmp)
	struct monst *mtmp;
{
	int turn;

	/* Shuffle the deck and deal */
	gypsy_charge(mtmp, TAROCCHI_COST);
	card_shuffle(mtmp);

	/* Play the given number of turns */
	for (turn = TAROCCHI_HAND; turn > 0; turn--) {
	}

	return;
}


/*** Monster-related functions ***/

void
gypsy_init (mtmp)
	struct monst *mtmp;
{
	mtmp->isgyp = TRUE;
	mtmp->mpeaceful = TRUE;
	mtmp->msleeping = 0;
	mtmp->mtrapseen = ~0;	/* traps are known */
	EGYP(mtmp)->credit = 0L;
	EGYP(mtmp)->top = 0;
	return;
}


void
gypsy_chat (mtmp)
	struct monst *mtmp;
{
	long money;
	winid win;
	anything any;
	menu_item *selected;
#ifdef GOLDOBJ
	long umoney;
#endif
	int n;

#ifdef GOLDOBJ
	umoney = money_cnt(invent);
#endif

	/* Sanity checks */
	if (!mtmp || !mtmp->mpeaceful || !mtmp->isgyp ||
			!humanoid(mtmp->data))
		return;

	/* Add up your available money */
	You("现在有%ld的zorkmid信用额度，同时身上有%ld的zorkmid。",
			EGYP(mtmp)->credit, plur(EGYP(mtmp)->credit),
#ifndef GOLDOBJ
			u.ugold, plur(u.ugold));
#else
			umoney, plur(umoney));			
#endif
	money = EGYP(mtmp)->credit +
#ifndef GOLDOBJ
											u.ugold;
#else
											umoney;
#endif

	/* Create the menu */
	any.a_void = 0;	/* zero out all bits */
	win = create_nhwindow(NHW_MENU);
	start_menu(win);

	/* Fortune */
	any.a_char = 'f';
	if (money >= FORTUNE_COST)
		add_menu(win, NO_GLYPH, &any , 'f', 0, ATR_NONE,
				"给你占卜一下", MENU_UNSELECTED);

	/* Three-card monte */
	any.a_char = 'm';
	if (money >= MONTE_COST)
		add_menu(win, NO_GLYPH, &any , 'm', 0, ATR_NONE,
				"三张赌一张", MENU_UNSELECTED);

	/* Ninety-nine */
	any.a_char = 'n';
	if (money >= NINETYNINE_COST)
		add_menu(win, NO_GLYPH, &any , 'n', 0, ATR_NONE,
				"吃墩", MENU_UNSELECTED);

	/* Pawn gems (always available) */
	any.a_char = 'p';
	add_menu(win, NO_GLYPH, &any , 'p', 0, ATR_NONE,
			"典当宝石", MENU_UNSELECTED);

	/* Yendorian Tarocchi */
	any.a_char = 't';
/*	if (money >= TAROCCHI_COST)
		add_menu(win, NO_GLYPH, &any , 't', 0, ATR_NONE,
				"", MENU_UNSELECTED);*/

	/* Help */
	any.a_char = '?';
		add_menu(win, NO_GLYPH, &any , '?', 0, ATR_NONE,
				"帮助", MENU_UNSELECTED);

	/* Display the menu */
	end_menu(win, "想玩哪个游戏？");
	n = select_menu(win, PICK_ONE, &selected);
	destroy_nhwindow(win);
	if (n > 0) switch (selected[0].item.a_char) {
		case 'f':
			fortune(mtmp);
			break;
		case 'm':
			monte(mtmp);
			break;
		case 'n':
			ninetynine(mtmp);
			break;
		case 'p':
			pawn(mtmp);
			break;
		case 't':
			tarocchi(mtmp);
			break;
		case '?':
			display_file_area(FILE_AREA_SHARE, "", TRUE);
			break;
	}

	return;
}

