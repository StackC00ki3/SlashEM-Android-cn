/*      SCCS Id: @(#)tech.c    3.2     98/Oct/30        */
/*      Original Code by Warren Cheung (Basis: spell.c, attrib.c) */
/*      Copyright (c) M. Stephenson 1988                          */
/* NetHack may be freely redistributed.  See license for details. */

/* All of the techs from cmd.c are ported here */

#include "hack.h"

/* #define DEBUG */		/* turn on for diagnostics */

static boolean FDECL(gettech, (int *));
static boolean FDECL(dotechmenu, (int, int *));
static void NDECL(doblitzlist);
static int FDECL(get_tech_no,(int));
static int FDECL(techeffects, (int));
static void FDECL(hurtmon, (struct monst *,int));
static int FDECL(mon_to_zombie, (int));
STATIC_PTR int NDECL(tinker);
STATIC_PTR int NDECL(draw_energy);
static const struct innate_tech * NDECL(role_tech);
static const struct innate_tech * NDECL(race_tech);
static int NDECL(doblitz);
static int NDECL(blitz_chi_strike);
static int NDECL(blitz_e_fist);
static int NDECL(blitz_pummel);
static int NDECL(blitz_g_slam);
static int NDECL(blitz_dash);
static int NDECL(blitz_power_surge);
static int NDECL(blitz_spirit_bomb);

static NEARDATA schar delay;            /* moves left for tinker/energy draw */
static NEARDATA const char revivables[] = { ALLOW_FLOOROBJ, FOOD_CLASS, 0 };

/* 
 * Do try to keep the names <= 25 chars long, or else the
 * menu will look bad :B  WAC
 */
 
STATIC_OVL NEARDATA const char *tech_names[] = {
	"no technique",
	"狂暴",
	"kiii",
	"research",
	"紧急手术",
	"强化记忆",
	"missile flurry",
	"操练武器",
	"eviscerate",
	"治愈之手",
	"安抚坐骑",
	"超度亡灵",
	"vanish",
	"割喉",
	"blessing",
	"元素之拳",
	"原始怒吼",
	"液体跳跃",
	"致命一击",
	"控制魔印",
	"爆破魔印",
	"释放魔印",
	"僵尸招来",
	"秽土转生",
	"火焰防御之壁！",
	"冰霜防御之壁",
	"闪电防御之壁",
	"锻造升级",
	"怒气爆发",
	"blink",
	"气功拳",
	"抽取能量",
	"气功治疗",
	"缴械",
	"dazzle",
	"地牢快打组合技",
	"pummel",
	"ground slam",
	"轻盈冲刺",
	"魔能浪涌",
	"意识震爆",
	"吸血",
	""
};

static const struct innate_tech 
	/* Roles */
	arc_tech[] = { {   1, T_RESEARCH, 1},
		       {   0, 0, 0} },
	bar_tech[] = { {   1, T_BERSERK, 1},
		       {   0, 0, 0} },
	cav_tech[] = { {   1, T_PRIMAL_ROAR, 1},
		       {   0, 0, 0} },
	fla_tech[] = { {   1, T_REINFORCE, 1},
		       {   3, T_POWER_SURGE, 1},
		       {   5, T_DRAW_ENERGY, 1},
		       {  10, T_SIGIL_TEMPEST, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },
	hea_tech[] = { {   1, T_SURGERY, 1},
		       {  20, T_REVIVE, 1},
		       {   0, 0, 0} },
	ice_tech[] = { {   1, T_REINFORCE, 1},
		       {   5, T_DRAW_ENERGY, 1},
		       {  10, T_SIGIL_TEMPEST, 1},
		       {  12, T_POWER_SURGE, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },
	kni_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_HEAL_HANDS, 1},
		       {   0, 0, 0} },
	mon_tech[] = { {   1, T_PUMMEL, 1},
		       {   1, T_DASH, 1},
		       {   1, T_BLITZ, 1},
		       {   2, T_CHI_STRIKE, 1},
	  	       {   4, T_CHI_HEALING, 1},
	  	       {   6, T_E_FIST, 1},
		       {   8, T_DRAW_ENERGY, 1},
		       {  10, T_G_SLAM, 1},
		       {  11, T_WARD_FIRE, 1},
		       {  13, T_WARD_COLD, 1},
		       {  15, T_WARD_ELEC, 1},
		       {  17, T_SPIRIT_BOMB, 1},
		       {  20, T_POWER_SURGE, 1},
		       {   0, 0, 0} },
	nec_tech[] = { {   1, T_REINFORCE, 1},
		       {   1, T_RAISE_ZOMBIES, 1},
		       {  10, T_POWER_SURGE, 1},
		       {  15, T_SIGIL_TEMPEST, 1},
		       {   0, 0, 0} },
	pri_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_BLESSING, 1},
		       {   0, 0, 0} },
	ran_tech[] = { {   1, T_FLURRY, 1},
		       {   0, 0, 0} },
	rog_tech[] = { {   1, T_CRIT_STRIKE, 1},
		       {  15, T_CUTTHROAT, 1},
		       {   0, 0, 0} },
	sam_tech[] = { {   1, T_KIII, 1},
		       {   0, 0, 0} },
	tou_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	und_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_PRACTICE, 1},
		       {   0, 0, 0} },
	val_tech[] = { {   1, T_PRACTICE, 1},
		       {   0, 0, 0} },
#ifdef YEOMAN
	yeo_tech[] = {
#ifdef STEED
		       {   1, T_CALM_STEED, 1},
#endif
		       {   0, 0, 0} },
#endif
	wiz_tech[] = { {   1, T_REINFORCE, 1},
		       {   3, T_DRAW_ENERGY, 1},
		       {   5, T_POWER_SURGE, 1},
		       {   7, T_SIGIL_CONTROL, 1},
		       {  14, T_SIGIL_TEMPEST, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },		       
	/* Races */
	dop_tech[] = { {   1, T_LIQUID_LEAP, 1},
		       {   0, 0, 0} },
	dwa_tech[] = { {   1, T_RAGE, 1},
		       {   0, 0, 0} },
	elf_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	gno_tech[] = { {   1, T_VANISH, 1},
		       {   7, T_TINKER, 1},
		       {   0, 0, 0} },
	hob_tech[] = { {   1, T_BLINK, 1},
		       {   0, 0, 0} },
	lyc_tech[] = { {   1, T_EVISCERATE, 1},
		       {  10, T_BERSERK, 1},
		       {   0, 0, 0} },
	vam_tech[] = { {   1, T_DAZZLE, 1},
		       {   1, T_DRAW_BLOOD, 1},
		       {   0, 0, 0} };
	/* Orc */

/* Local Macros 
 * these give you direct access to the player's list of techs.  
 * Are you sure you don't want to use tech_inuse,  which is the
 * extern function for checking whether a fcn is inuse
 */

#define techt_inuse(tech)       tech_list[tech].t_inuse
#define techtout(tech)        tech_list[tech].t_tout
#define techlev(tech)         (u.ulevel - tech_list[tech].t_lev)
#define techid(tech)          tech_list[tech].t_id
#define techname(tech)        (tech_names[techid(tech)])
#define techlet(tech)  \
        ((char)((tech < 26) ? ('a' + tech) : ('A' + tech - 26)))

/* A simple pseudorandom number generator
 *
 * This should generate fairly random numbers that will be 
 * mod LP_HPMOD from 2 to 9,  with 0 mod LP_HPMOD 
 * but can't use the normal RNG since can_limitbreak() must
 * return the same state on the same turn.
 * This also has to depend on things that do NOT change during 
 * save and restore,  and also should only change between turns
 */
#if 0 /* Probably overkill */
#define LB_CYCLE 259993L	/* number of turns before the pattern repeats */
#define LB_BASE1 ((long) (monstermoves + u.uhpmax + 300L))
#define LB_BASE2 ((long) (moves + u.uenmax + u.ulevel + 300L))
#define LB_STRIP 6	/* Remove the last few bits as they tend to be less random */
#endif
 
#define LB_CYCLE 101L	/* number of turns before the pattern repeats */
#define LB_BASE1 ((long) (monstermoves + u.uhpmax + 10L))
#define LB_BASE2 ((long) (moves + u.uenmax + u.ulevel + 10L))
#define LB_STRIP 3	/* Remove the last few bits as they tend to be less random */
 
#define LB_HPMOD ((long) ((u.uhp * 10 / u.uhpmax > 2) ? \
        			(u.uhp * 10 / u.uhpmax) : 2))

#define can_limitbreak() (!Upolyd && (u.uhp*10 < u.uhpmax) && \
        		  (u.uhp == 1 || (!((((LB_BASE1 * \
        		  LB_BASE2) % LB_CYCLE) >> LB_STRIP) \
        		  % LB_HPMOD))))
        
/* Whether you know the tech */
boolean
tech_known(tech)
	short tech;
{
	int i;
	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) 
		     return TRUE;
	}
	return FALSE;
}

/* Called to prematurely stop a technique */
void
aborttech(tech)
{
	int i;

	i = get_tech_no(tech);
	if (tech_list[i].t_inuse) {
	    switch (tech_list[i].t_id) {
		case T_RAGE:
		    u.uhpmax -= tech_list[i].t_inuse - 1;
		    if (u.uhpmax < 1)
			u.uhpmax = 0;
		    u.uhp -= tech_list[i].t_inuse - 1;
		    if (u.uhp < 1)
			u.uhp = 1;
		    break;
		case T_POWER_SURGE:
		    u.uenmax -= tech_list[i].t_inuse - 1;
		    if (u.uenmax < 1)
			u.uenmax = 0;
		    u.uen -= tech_list[i].t_inuse - 1;
		    if (u.uen < 0)
			u.uen = 0;
		    break;
	    }
	    tech_list[i].t_inuse = 0;
	}
}

/* Called to teach a new tech.  Level is starting tech level */
void
learntech(tech, mask, tlevel)
	short tech;
	long mask;
	int tlevel;
{
	int i;
	const struct innate_tech *tp;

	i = get_tech_no(tech);
	if (tlevel > 0) {
	    if (i < 0) {
		i = get_tech_no(NO_TECH);
		if (i < 0) {
		    impossible("没有技术栏了？");
		    return;
		}
	    }
	    tlevel = u.ulevel ? u.ulevel - tlevel : 0;
	    if (tech_list[i].t_id == NO_TECH) {
		tech_list[i].t_id = tech;
		tech_list[i].t_lev = tlevel;
		tech_list[i].t_inuse = 0; /* not in use */
		tech_list[i].t_intrinsic = 0;
	    }
	    else if (tech_list[i].t_intrinsic & mask) {
		impossible("已经知道本技术了.");
		return;
	    }
	    if (mask == FROMOUTSIDE) {
		tech_list[i].t_intrinsic &= ~OUTSIDE_LEVEL;
		tech_list[i].t_intrinsic |= tlevel & OUTSIDE_LEVEL;
	    }
	    if (tlevel < tech_list[i].t_lev)
		tech_list[i].t_lev = tlevel;
	    tech_list[i].t_intrinsic |= mask;
	    tech_list[i].t_tout = 0; /* Can use immediately*/
	}
	else if (tlevel < 0) {
	    if (i < 0 || !(tech_list[i].t_intrinsic & mask)) {
		impossible("未知技术。");
		return;
	    }
	    tech_list[i].t_intrinsic &= ~mask;
	    if (!(tech_list[i].t_intrinsic & INTRINSIC)) {
		if (tech_list[i].t_inuse)
		    aborttech(tech);
		tech_list[i].t_id = NO_TECH;
		return;
	    }
	    /* Re-calculate lowest t_lev */
	    if (tech_list[i].t_intrinsic & FROMOUTSIDE)
		tlevel = tech_list[i].t_intrinsic & OUTSIDE_LEVEL;
	    if (tech_list[i].t_intrinsic & FROMEXPER) {
		for(tp = role_tech(); tp->tech_id; tp++)
		    if (tp->tech_id == tech)
			break;
		if (!tp->tech_id)
		    impossible("没有该种族的内置技术？");
		else if (tlevel < 0 || tp->ulevel - tp->tech_lev < tlevel)
		    tlevel = tp->ulevel - tp->tech_lev;
	    }
	    if (tech_list[i].t_intrinsic & FROMRACE) {
		for(tp = race_tech(); tp->tech_id; tp++)
		    if (tp->tech_id == tech)
			break;
		if (!tp->tech_id)
		    impossible("");
		else if (tlevel < 0 || tp->ulevel - tp->tech_lev < tlevel)
		    tlevel = tp->ulevel - tp->tech_lev;
	    }
	    tech_list[i].t_lev = tlevel;
	}
	else
	    impossible("技术等级错误！");
}

/*
 * Return TRUE if a tech was picked, with the tech index in the return
 * parameter.  Otherwise return FALSE.
 */
static boolean
gettech(tech_no)
        int *tech_no;
{
        int i, ntechs, idx;
	char ilet, lets[BUFSZ], qbuf[QBUFSZ];

	for (ntechs = i = 0; i < MAXTECH; i++)
	    if (techid(i) != NO_TECH) ntechs++;
	if (ntechs == 0)  {
            You("现在没有掌握任何技巧。");
	    return FALSE;
	}
	if (flags.menu_style == MENU_TRADITIONAL) {
            if (ntechs == 1)  Strcpy(lets, "a");
            else if (ntechs < 27)  Sprintf(lets, "a-%c", 'a' + ntechs - 1);
            else if (ntechs == 27)  Sprintf(lets, "a-z A");
            else Sprintf(lets, "a-z A-%c", 'A' + ntechs - 27);

	    for(;;)  {
                Sprintf(qbuf, "想要施展什么技术？ [%s ?]", lets);
		if ((ilet = yn_function(qbuf, (char *)0, '\0')) == '?')
		    break;

		if (index(quitchars, ilet))
		    return FALSE;

		if (letter(ilet) && ilet != '@') {
		    /* in a-zA-Z, convert back to an index */
		    if (lowc(ilet) == ilet)     /* lower case */
			idx = ilet - 'a';
		    else
			idx = ilet - 'A' + 26;

                    if (idx < ntechs)
			for(i = 0; i < MAXTECH; i++)
			    if (techid(i) != NO_TECH) {
				if (idx-- == 0) {
				    *tech_no = i;
				    return TRUE;
				}
			    }
		}
                You("不知道那个技术怎么施展。");
	    }
	}
        return dotechmenu(PICK_ONE, tech_no);
}

static boolean
dotechmenu(how, tech_no)
	int how;
        int *tech_no;
{
	winid tmpwin;
	int i, n, len, longest, techs_useable, tlevel;
	char buf[BUFSZ], let = 'a';
	const char *prefix;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

	techs_useable = 0;

	if (!iflags.menu_tab_sep) {
	    /* find the length of the longest tech */
	    for (longest = 0, i = 0; i < MAXTECH; i++) {
		if (techid(i) == NO_TECH) continue;
		if ((len = strlen(techname(i))) > longest)
		    longest = len;
	    }
	    Sprintf(buf, "    %-*s Level   Status", longest, "Name");
	} else
	    Sprintf(buf, "Name\tLevel\tStatus");

	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

	for (i = 0; i < MAXTECH; i++) {
	    if (techid(i) == NO_TECH)
		continue;
	    tlevel = techlev(i);
	    if (!techtout(i) && tlevel > 0) {
		/* Ready to use */
		techs_useable++;
		prefix = "";
		any.a_int = i + 1;
	    } else {
		prefix = "";
		any.a_int = 0;
	    }
#ifdef WIZARD
	    if (wizard) 
		if (!iflags.menu_tab_sep)			
		    Sprintf(buf, "%s%-*s %2d%c%c%c   %s(%i)",
			    prefix, longest, techname(i), tlevel,
			    tech_list[i].t_intrinsic & FROMEXPER ? 'X' : ' ',
			    tech_list[i].t_intrinsic & FROMRACE ? 'R' : ' ',
			    tech_list[i].t_intrinsic & FROMOUTSIDE ? 'O' : ' ',
			    tech_inuse(techid(i)) ? "正在施展中" :
			    tlevel <= 0 ? "无法回忆" :
			    can_limitbreak() ? "可极限激发" :
			    !techtout(i) ? "可施展" : 
			    techtout(i) > 100 ? "冷却中" : "Soon",
			    techtout(i));
		else
		    Sprintf(buf, "%s%s\t%2d%c%c%c\t%s(%i)",
			    prefix, techname(i), tlevel,
			    tech_list[i].t_intrinsic & FROMEXPER ? 'X' : ' ',
			    tech_list[i].t_intrinsic & FROMRACE ? 'R' : ' ',
			    tech_list[i].t_intrinsic & FROMOUTSIDE ? 'O' : ' ',
			    tech_inuse(techid(i)) ? "正在施展中" :
			    tlevel <= 0 ? "无法回忆" :
			    can_limitbreak() ? "可极限激发" :
			    !techtout(i) ? "可施展" : 
			    techtout(i) > 100 ? "冷却中" : "Soon",
			    techtout(i));
	    else
#endif
	    if (!iflags.menu_tab_sep)			
		Sprintf(buf, "%s%-*s %5d   %s",
			prefix, longest, techname(i), tlevel,
			tech_inuse(techid(i)) ? "正在施展中" :
			tlevel <= 0 ? "无法回忆" :
			can_limitbreak() ? "可极限激发" :
			!techtout(i) ? "可施展" : 
			techtout(i) > 100 ? "冷却中" : "Soon");
	    else
		Sprintf(buf, "%s%s\t%5d\t%s",
			prefix, techname(i), tlevel,
			tech_inuse(techid(i)) ? "正在施展中" :
			tlevel <= 0 ? "无法回忆" :
			can_limitbreak() ? "可极限激发" :
			!techtout(i) ? "可施展" : 
			techtout(i) > 100 ? "冷却中" : "Soon");

	    add_menu(tmpwin, NO_GLYPH, &any,
		    techtout(i) ? 0 : let, 0, ATR_NONE, buf, MENU_UNSELECTED);
	    if (let++ == 'z') let = 'A';
	}

	if (!techs_useable) 
	    how = PICK_NONE;

	end_menu(tmpwin, how == PICK_ONE ? "请选择一个要施展的技巧" :
					   "当前已掌握的技术");

	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *tech_no = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

static int
get_tech_no(tech)
int tech;
{
	int i;

	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) {
			return(i);
		}
	}
	return (-1);
}

int
dotech()
{
	int tech_no;

	if (gettech(&tech_no))
	    return techeffects(tech_no);
	return 0;
}

static NEARDATA const char kits[] = { TOOL_CLASS, 0 };

static struct obj *
use_medical_kit(type, feedback, verb)
int type;
boolean feedback;
char *verb;
{
    struct obj *obj, *otmp;
    makeknown(MEDICAL_KIT);
    if (!(obj = carrying(MEDICAL_KIT))) {
	if (feedback) You("需要一个医疗套装才能这么做。");
	return (struct obj *)0;
    }
    for (otmp = invent; otmp; otmp = otmp->nobj)
	if (otmp->otyp == MEDICAL_KIT && otmp != obj)
	    break;
    if (otmp) {	/* More than one medical kit */
	obj = getobj(kits, verb);
	if (!obj)
	    return (struct obj *)0;
    }
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	if (otmp->otyp == type)
	    break;
    if (!otmp) {
	if (feedback)
	    You_cant("在%s里面找到更多的%s了。",
			yname(obj)), makeplural(simple_typename(type));
	return (struct obj *)0;
    }
    return otmp;
}

/* gettech is reworked getspell */
/* reworked class special effects code */
/* adapted from specialpower in cmd.c */
static int
techeffects(tech_no)
int tech_no;
{
	/* These variables are used in various techs */
	struct obj *obj, *otmp;
	const char *str;
	struct monst *mtmp;
	int num;
	char Your_buf[BUFSZ];
	char allowall[2];
	int i, j, t_timeout = 0;


	/* check timeout */
	if (tech_inuse(techid(tech_no))) {
	    pline("你已经激活了这个技术！");
	    return (0);
	}
        if (techtout(tech_no) && !can_limitbreak()) {
	    You("必须要%s才能再次使用技巧。",
                (techtout(tech_no) > 100) ?
                        "for a while" : "a little longer");
#ifdef WIZARD
            if (!wizard || (yn("仍要使用技术吗？") == 'n'))
#endif
                return(0);
        }

	/* switch to the tech and do stuff */
        switch (techid(tech_no)) {
            case T_RESEARCH:
		/* WAC stolen from the spellcasters...'A' can identify from
        	   historical research*/
		if(Hallucination || Stunned || Confusion) {
		    You("现在没法集中精神！");
		    return(0);
		} else if((ACURR(A_INT) + ACURR(A_WIS)) < rnd(60)) {
			pline("你包里头的东西好像看起来都不怎么熟悉。");
                    t_timeout = rn1(500,500);
		    break;
		} else if(invent) {
			You("检查了一下你的随身物品。");
			identify_pack((int) ((techlev(tech_no) / 10) + 1));
		} else {
			/* KMH -- fixed non-compliant string */
		    You("已经很熟悉包里的所有物品了。");
		    break;
		}
                t_timeout = rn1(500,1500);
		break;
            case T_EVISCERATE:
		/* only when empty handed, in human form */
		if (Upolyd || uwep || uarmg) {
		    You_cant("在%s的时候这么干！", Upolyd ? "被变形的" :
			    uwep ? "holding a weapon" : "戴着手套的");
		    return 0;
		}
		Your("指甲变长，变成了金刚狼一样的爪子！");
		aggravate();
		techt_inuse(tech_no) = d(2,4) + techlev(tech_no)/5 + 2;
		t_timeout = rn1(1000,1000);
		break;
            case T_BERSERK:
		You("瞬间进入了狂战士的狂暴状态！");
		techt_inuse(tech_no) = d(2,8) +
               		(techlev(tech_no)/5) + 2;
		incr_itimeout(&HFast, techt_inuse(tech_no));
		t_timeout = rn1(1000,500);
		break;
            case T_REINFORCE:
		/* WAC spell-users can study their known spells*/
		if(Hallucination || Stunned || Confusion) {
		    You("现在没法集中精神！");
		    break;
               	} else {
		    You("聚精会神……");
		    if (studyspell()) t_timeout = rn1(1000,500); /*in spell.c*/
		}
               break;
            case T_FLURRY:
                Your("%s %s become blurs as they reach for your quiver!",
			uarmg ? "戴着手套的" : "光着的",      /* Del Lamb */
			makeplural(body_part(HAND)));
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
                t_timeout = rn1(1000,500);
		break;
            case T_PRACTICE:
                if(!uwep || (weapon_type(uwep) == P_NONE)) {
		    You("没有拿着武器！");
		    return(0);
		} else if(uwep->known == TRUE) {
                    practice_weapon();
		} else {
                    if (not_fully_identified(uwep)) {
                        You("仔细检查了%s。", doname(uwep));
                            if (rnd(15) <= ACURR(A_INT)) {
                                makeknown(uwep->otyp);
                                uwep->known = TRUE;
                                You("认出来它其实是%s。",doname(uwep));
                                } else
                     pline("不幸的是你没学到什么新东西。");
                    } 
                /*WAC Added practicing code - in weapon.c*/
                    practice_weapon();
		}
                t_timeout = rn1(500,500);
		break;
            case T_SURGERY:
		if (Hallucination || Stunned || Confusion) {
		    You("现在没有进行手术的条件！");
		    break;
		}
		if (Sick || Slimed) {
		    if (carrying(SCALPEL)) {
			pline("你用手术刀（好几把疼啊！）治好了自己的感染！");
			make_sick(0L, (char *)0, TRUE, SICK_ALL);
			Slimed = 0;
			if (Upolyd) {
			    u.mh -= 5;
			    if (u.mh < 1)
				rehumanize();
			} else if (u.uhp > 6)
			    u.uhp -= 5;
			else
			    u.uhp = 1;
                        t_timeout = rn1(500,500);
			flags.botl = TRUE;
			break;
		    } else pline("前提是你手里得要有把手术刀才行啊……");
		}
		if (Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax) {
		    otmp = use_medical_kit(BANDAGE, FALSE,
			    "dress your wounds with");
		    if (otmp) {
			check_unpaid(otmp);
			if (otmp->quan > 1L) {
			    otmp->quan--;
			    otmp->ocontainer->owt = weight(otmp->ocontainer);
			} else {
			    obj_extract_self(otmp);
			    obfree(otmp, (struct obj *)0);
			}
			pline("你用%s处理了自己的伤口。", yname(otmp));
			healup(techlev(tech_no) * (rnd(2)+1) + rn1(5,5),
			  0, FALSE, FALSE);
		    } else {
			You("尽全力缝合了自己的伤口。");
			healup(techlev(tech_no) + rn1(5,5), 0, FALSE, FALSE);
		    }
                    t_timeout = rn1(1000,500);
		    flags.botl = TRUE;
		} else You("现在不需要这股治愈之力！");
		break;
            case T_HEAL_HANDS:
		if (Slimed) {
		    Your("身体着火了！");
		    burn_away_slime();
		    t_timeout = 3000;
		} else if (Sick) {
		    You("把你的手放到了严重感染的部位上……");
		    make_sick(0L, (char*)0, TRUE, SICK_ALL);
		    t_timeout = 3000;
		} else if (Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax) {
		    pline("一股暖流突然涌进你的身体！");
		    healup(techlev(tech_no) * 4, 0, FALSE, FALSE);
		    t_timeout = 3000;
		} else
		    pline(nothing_happens);
		break;
            case T_KIII:
		You("大声喊道：“杀呀！！！！”");
		aggravate();
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
                t_timeout = rn1(1000,500);
		break;
#ifdef STEED
	    case T_CALM_STEED:
                if (u.usteed) {
                        pline("%s的驯服程度更深了一点。", Monnam(u.usteed));
                        tamedog(u.usteed, (struct obj *) 0);
                        t_timeout = rn1(1000,500);
                } else
                        Your("这个技术只在骑着怪物时才能使用。");
                break;
#endif
            case T_TURN_UNDEAD:
                return(turn_undead());
	    case T_VANISH:
		if (Invisible && Fast) {
			You("已经很灵活很让人摸不清位置了。");
		}
                techt_inuse(tech_no) = rn1(50,50) + techlev(tech_no);
		if (!Invisible) pline("你的身影在一阵烟雾中消失了！");
		if (!Fast) You("感觉更加灵活了！");
		incr_itimeout(&HInvis, techt_inuse(tech_no));
		incr_itimeout(&HFast, techt_inuse(tech_no));
		newsym(u.ux,u.uy);      /* update position */
		t_timeout = rn1(1000,500);
		break;
	    case T_CRIT_STRIKE:
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
		    /* Hopefully a mistake ;B */
		    You("决定还是算了吧。");
		    return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
		    You("perform a flashy twirl!");
		    return (0);
		} else {
		    int oldhp = mtmp->mhp;
		    int tmp;

		    if (!attack(mtmp)) return(0);
		    if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp &&
			    !noncorporeal(mtmp->data) && !unsolid(mtmp->data)) {
			You("狠狠地打了%s的内脏！", s_suffix(mon_nam(mtmp)));
			/* Base damage is always something, though it may be
			 * reduced to zero if the hero is hampered. However,
			 * since techlev will never be zero, stiking vital
			 * organs will always do _some_ damage.
			 */
			tmp = mtmp->mhp > 1 ? mtmp->mhp / 2 : 1;
			if (!humanoid(mtmp->data) || is_golem(mtmp->data) ||
				mtmp->data->mlet == S_CENTAUR) {
			    You("因为在解剖学上找不到脖子而收手了。");
			    tmp /= 2;
			}
			tmp += techlev(tech_no);
			t_timeout = rn1(1000, 500);
			hurtmon(mtmp, tmp);
		    }
		}
		break;
	    case T_CUTTHROAT:
		if (!is_blade(uwep)) {
		    You("需要一把带刃的武器才能施展割喉技术！");
		    return 0;
		}
	    	if (!getdir((char *)0)) return 0;
		if (!u.dx && !u.dy) {
		    /* Hopefully a mistake ;B */
		    pline("我知道你最近有很多烦心事，但你也没必要自杀吧。");
		    return 0;
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
		    You("攻击了……什么也没攻击到！");
		    return 0;
		} else {
		    int oldhp = mtmp->mhp;

		    if (!attack(mtmp)) return 0;
		    if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp) {
			if (!has_head(mtmp->data) || u.uswallow)
			    You_cant("对%s施展了割喉！", mon_nam(mtmp));
			else {
			    int tmp = 0;

			    if (rn2(5) < (techlev(tech_no)/10 + 1)) {
				You("sever %s head!", s_suffix(mon_nam(mtmp)));
				tmp = mtmp->mhp;
			    } else {
				You("狠狠地伤害了%s！", s_suffix(mon_nam(mtmp)));
				tmp = mtmp->mhp / 2;
			    }
			    tmp += techlev(tech_no);
			    t_timeout = rn1(1000,500);
			    hurtmon(mtmp, tmp);
			}
		    }
		}
		break;
	    case T_BLESSING:
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		
		if ( !(obj = getobj(allowall, "祝福哪个？"))) return(0);
		pline("你手中涌现出一股圣洁的光芒！");
                if (!Blind) (void) Shk_Your(Your_buf, obj);
		if (obj->cursed) {
                	if (!Blind)
                    		pline("%s %s %s.",Your_buf,
						  aobjnam(obj, "softly glow"),
						  hcolor(NH_AMBER));
				uncurse(obj);
				obj->bknown=1;
		} else if(!obj->blessed) {
			if (!Blind) {
				str = hcolor(NH_LIGHT_BLUE);
				pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  index(vowels, *str) ? "n" : "", str);
			}
			bless(obj);
			obj->bknown=1;
		} else {
			if (obj->bknown) {
				pline ("这个物品已经是受祝福状态了！");
				return(0);
			}
			obj->bknown=1;
			pline("这股光芒消失了。");
		}
		t_timeout = rn1(1000,500);
		break;
	    case T_E_FIST: 
	    	blitz_e_fist();
#if 0
		str = makeplural(body_part(HAND));
                You("用意志导引元素之力，然后灌注进你的%s里", str);
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/3 + 1)) + d(1,4) + 2;
#endif
		t_timeout = rn1(1000,500);
	    	break;
	    case T_PRIMAL_ROAR:	    	
	    	You("放出了能把敌人吓破胆的嘶吼声！");
	    	aggravate();

		techt_inuse(tech_no) = d(2,6) + (techlev(tech_no)) + 2;

		incr_itimeout(&HFast, techt_inuse(tech_no));

	    	for(i = -5; i <= 5; i++) for(j = -5; j <= 5; j++)
		    if(isok(u.ux+i, u.uy+j) && (mtmp = m_at(u.ux+i, u.uy+j))) {
		    	if (mtmp->mtame != 0 && !mtmp->isspell) {
		    	    struct permonst *ptr = mtmp->data;
			    struct monst *mtmp2;
		    	    int ttime = techt_inuse(tech_no);
		    	    int type = little_to_big(monsndx(ptr));
		    	    
		    	    mtmp2 = tamedog(mtmp, (struct obj *) 0);
			    if (mtmp2)
				mtmp = mtmp2;

		    	    if (type && type != monsndx(ptr)) {
				ptr = &mons[type];
		    	    	mon_spec_poly(mtmp, ptr, ttime, FALSE,
					canseemon(mtmp), FALSE, TRUE);
		    	    }
		    	}
		    }
		t_timeout = rn1(1000,500);
	    	break;
	    case T_LIQUID_LEAP: {
	    	coord cc;
	    	int dx, dy, sx, sy, range;

		pline("你想跳到哪里？");
    		cc.x = sx = u.ux;
		cc.y = sy = u.uy;

		getpos(&cc, TRUE, "目标位置");
		if (cc.x == -10) return 0; /* user pressed esc */

		dx = cc.x - u.ux;
		dy = cc.y - u.uy;
		/* allow diagonals */
	    	if (dx && dy && dx != dy && dx != -dy) {
		    You("只能在直线上进行跳跃！");
		    return 0;
	    	} else if (distu(cc.x, cc.y) > 19 + techlev(tech_no)) {
		    pline("太远了！");
		    return 0;
		} else if (m_at(cc.x, cc.y) || !isok(cc.x, cc.y) ||
			IS_ROCK(levl[cc.x][cc.y].typ) ||
			sobj_at(BOULDER, cc.x, cc.y) ||
			closed_door(cc.x, cc.y)) {
		    You_cant("flow there!"); /* MAR */
		    return 0;
		} else {
		    You("全身都流体化了！");
		    if (Punished) {
			You("滑出了铁链上卡着你脚的锁。");
			unpunish();
		    }
		    if(u.utrap) {
			switch(u.utraptype) {
			    case TT_BEARTRAP: 
				You("的身体流出了捕兽夹。");
				break;
			    case TT_PIT:
				You("从坑里跳了出来！");
				break;
			    case TT_WEB:
				You("直接穿过了蜘蛛网！");
				break;
			    case TT_LAVA:
				You("和周边的岩浆分离了！");
				u.utrap = 0;
				break;
			    case TT_INFLOOR:
				u.utrap = 0;
				You("流出了地板！");
			}
			u.utrap = 0;
		    }
		    /* Fry the things in the path ;B */
		    if (dx) range = dx;
		    else range = dy;
		    if (range < 0) range = -range;
		    
		    dx = sgn(dx);
		    dy = sgn(dy);
		    
		    while (range-- > 0) {
		    	int tmp_invul = 0;
		    	
		    	if (!Invulnerable) Invulnerable = tmp_invul = 1;
			sx += dx; sy += dy;
			tmp_at(DISP_BEAM, zapdir_to_glyph(dx, dy, AD_ACID-1));
			tmp_at(sx,sy);
			delay_output(); /* wait a little */
		    	if ((mtmp = m_at(sx, sy)) != 0) {
			    int chance;
			    
			    chance = rn2(20);
		    	    if (!chance || (3 - chance) > AC_VALUE(find_mac(mtmp)))
		    	    	break;
			    setmangry(mtmp);
		    	    You("catch %s in your acid trail!", mon_nam(mtmp));
		    	    if (!resists_acid(mtmp)) {
				int tmp = 1;
				/* Need to add a to-hit */
				tmp += d(2,4);
				tmp += rn2((int) (techlev(tech_no)/5 + 1));
				if (!Blind) pline_The("酸液烧伤了%s！", mon_nam(mtmp));
				hurtmon(mtmp, tmp);
			    } else if (!Blind) pline_The("酸液没有影响到%s！", mon_nam(mtmp));
			}
			/* Clean up */
			tmp_at(DISP_END,0);
			if (tmp_invul) Invulnerable = 0;
		    }

		    /* A little Sokoban guilt... */
		    if (In_sokoban(&u.uz))
			change_luck(-1);
		    You("又重新固化了！");
		    teleds(cc.x, cc.y, FALSE);
		    nomul(-1);
		    nomovemsg = "";
	    	}
		t_timeout = rn1(1000,500);
	    	break;
	    }
            case T_SIGIL_TEMPEST: 
		/* Have enough power? */
		num = 50 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("没有足够激活魔印的能量！");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("激活了爆破魔印！");
                techt_inuse(tech_no) = d(1,6) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_SIGIL_CONTROL:
		/* Have enough power? */
		num = 30 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("没有足够激活魔印的能量！");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("激活了控制魔印！");
                techt_inuse(tech_no) = d(1,4) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_SIGIL_DISCHARGE:
		/* Have enough power? */
		num = 100 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("没有足够激活魔印的能量！");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("激活了释放魔印！");
                techt_inuse(tech_no) = d(1,4) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_RAISE_ZOMBIES:
            	You("吟唱了远古时期的诅咒咒文……");
		for(i = -1; i <= 1; i++) for(j = -1; j <= 1; j++) {
		    int corpsenm;

		    if (!isok(u.ux+i, u.uy+j)) continue;
		    for (obj = level.objects[u.ux+i][u.uy+j]; obj; obj = otmp) {
			otmp = obj->nexthere;

			if (obj->otyp != CORPSE) continue;
			/* Only generate undead */
			corpsenm = mon_to_zombie(obj->corpsenm);
			if (corpsenm != -1 && !cant_create(&corpsenm, TRUE) &&
			  (!obj->oxlth || obj->oattached != OATTACHED_MONST)) {
			    /* Maintain approx. proportion of oeaten to cnutrit
			     * so that the zombie's HP relate roughly to how
			     * much of the original corpse was left.
			     */
			    if (obj->oeaten)
				obj->oeaten =
					eaten_stat(mons[corpsenm].cnutrit, obj);
			    obj->corpsenm = corpsenm;
			    mtmp = revive(obj);
			    if (mtmp) {
				if (!resist(mtmp, SPBOOK_CLASS, 0, TELL)) {
				   mtmp = tamedog(mtmp, (struct obj *) 0);
				   You("掌控了%s！", mon_nam(mtmp));
				} else setmangry(mtmp);
			    }
			}
		    }
		}
		nomul(-2); /* You need to recover */
		nomovemsg = 0;
		t_timeout = rn1(1000,500);
		break;
            case T_REVIVE: 
		if (u.uswallow) {
		    You(no_elbow_room);
		    return 0;
		}
            	num = 100 - techlev(tech_no); /* WAC make this depend on mon? */
            	if ((Upolyd && u.mh <= num) || (!Upolyd && u.uhp <= num)){
		    You("don't have the strength to perform revivification!");
		    return 0;
            	}

            	obj = getobj((const char *)revivables, "revive");
            	if (!obj) return (0);
            	mtmp = revive(obj);
            	if (mtmp) {
#ifdef BLACKMARKET
		    if (Is_blackmarket(&u.uz))
			setmangry(mtmp);
		    else
#endif
		    if (mtmp->isshk)
			make_happy_shk(mtmp, FALSE);
		    else if (!resist(mtmp, SPBOOK_CLASS, 0, NOTELL))
			(void) tamedog(mtmp, (struct obj *) 0);
		}
            	if (Upolyd) u.mh -= num;
            	else u.uhp -= num;
		t_timeout = rn1(1000,500);
            	break;
	    case T_WARD_FIRE:
		/* Already have it intrinsically? */
		if (HFire_resistance & FROMOUTSIDE) return (0);

		You("激活了火焰防御之壁！");
		HFire_resistance += rn1(100,50);
		HFire_resistance += techlev(tech_no);
		t_timeout = rn1(1000,500);

	    	break;
	    case T_WARD_COLD:
		/* Already have it intrinsically? */
		if (HCold_resistance & FROMOUTSIDE) return (0);

		You("激活了冰霜防御之壁！");
		HCold_resistance += rn1(100,50);
		HCold_resistance += techlev(tech_no);
		t_timeout = rn1(1000,500);

	    	break;
	    case T_WARD_ELEC:
		/* Already have it intrinsically? */
		if (HShock_resistance & FROMOUTSIDE) return (0);

		You("激活了闪电防御之壁！");
		HShock_resistance += rn1(100,50);
		HShock_resistance += techlev(tech_no);
		t_timeout = rn1(1000,500);

	    	break;
	    case T_TINKER:
		if (Blind) {
			You("如果看不见的话就没法升级任何东西！");
			return (0);
		}
		if (!uwep) {
			You("没拿着任何可以升级的物品！");
			return (0);
		}
		You("正拿着%s。", doname(uwep));
		if (yn("想要升级这件物品么？") != 'y') return(0);
		You("开始%s。",doname(uwep));
		delay=-150 + techlev(tech_no);
		set_occupation(tinker, "进行升级", 0);
		break;
	    case T_RAGE:     	
		if (Upolyd) {
			You("没法聚精会神的激发愤怒！");
			return(0);
		}
	    	You("感觉这股心中的怒火如惊涛骇浪般爆发！");
		num = 50 + (4 * techlev(tech_no));
	    	techt_inuse(tech_no) = num + 1;
		u.uhpmax += num;
		u.uhp += num;
		t_timeout = rn1(1000,500);
		break;	    
	    case T_BLINK:
	    	You("感觉时间的流速突然慢了下来。");
                techt_inuse(tech_no) = rnd(techlev(tech_no) + 1) + 2;
		t_timeout = rn1(1000,500);
	    	break;
            case T_CHI_STRIKE:
            	if (!blitz_chi_strike()) return(0);
                t_timeout = rn1(1000,500);
		break;
            case T_DRAW_ENERGY:
            	if (u.uen == u.uenmax) {
            		if (Hallucination) You("已经充满能量了！");
			else You("没法再吸收更多能量了！");
			return(0);           		
            	}
                You("开始从周边的环境中抽取能量！");
		delay=-15;
		set_occupation(draw_energy, "抽取能量", 0);                
                t_timeout = rn1(1000,500);
		break;
            case T_CHI_HEALING:
            	if (u.uen < 1) {
            		You("过于虚弱，无法这么做！");
            		return(0);
            	}
		You("导引浑身能量，然后用这股能量治愈了自己的身体！");
                techt_inuse(tech_no) = techlev(tech_no)*2 + 4;
                t_timeout = rn1(1000,500);
		break;	
	    case T_DISARM:
	    	if (P_SKILL(weapon_type(uwep)) == P_NONE) {
	    		You("手里没拿着一个属于武器的物品！");
	    		return(0);
	    	}
	    	if ((P_SKILL(weapon_type(uwep)) < P_SKILLED) || (Blind)) {
	    		You("现在不能做这个！");
	    		return(0);
	    	}
		if (u.uswallow) {
	    		pline("你觉得你是谁？吞剑的卖艺小丑？",
				mon_nam(u.ustuck));
	    		return(0);
		}

	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			pline("你为什么不试试拿别的东西呢。");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp || !canspotmon(mtmp)) {
			if (memory_is_invisible(u.ux + u.dx, u.uy + u.dy))
			    You("不知道该瞄准哪里！");
			else
			    You("在那什么也没看见！");
			return (0);
		}
	    	obj = MON_WEP(mtmp);   /* can be null */
	    	if (!obj) {
	    		You_cant("缴械了一个本来就没拿着武器的敌人！");
	    		return(0);
	    	}
		/* Blindness dealt with above */
		if (!mon_visible(mtmp)
#ifdef INVISIBLE_OBJECTS
				|| obj->oinvis && !See_invisible
#endif
				) {
	    		You_cant("看见%s的武器！", s_suffix(mon_nam(mtmp)));
	    		return(0);
		}
		num = ((rn2(techlev(tech_no) + 15)) 
			* (P_SKILL(weapon_type(uwep)) - P_SKILLED + 1)) / 10;

		You("试图对%s进行缴械……",mon_nam(mtmp));
		/* WAC can't yank out cursed items */
                if (num > 0 && (!Fumbling || !rn2(10)) && !obj->cursed) {
		    int roll;
		    obj_extract_self(obj);
		    possibly_unwield(mtmp, FALSE);
		    setmnotwielded(mtmp, obj);
		    roll = rn2(num + 1);
		    if (roll > 3) roll = 3;
		    switch (roll) {
			case 2:
			    /* to floor near you */
			    You("knock %s %s to the %s!",
				s_suffix(mon_nam(mtmp)),
				xname(obj),
				surface(u.ux, u.uy));
			    if (obj->otyp == CRYSKNIFE &&
				    (!obj->oerodeproof || !rn2(10))) {
				obj->otyp = WORM_TOOTH;
				obj->oerodeproof = 0;
			    }
			    place_object(obj, u.ux, u.uy);
			    stackobj(obj);
			    break;
			case 3:
#if 0
			    if (!rn2(25)) {
				/* proficient at disarming, but maybe not
				   so proficient at catching weapons */
				int hitu, hitvalu;

				hitvalu = 8 + obj->spe;
				hitu = thitu(hitvalu,
					dmgval(obj, &youmonst),
					obj, xname(obj));
				if (hitu)
				    pline("",
					    The(xname(obj)));
				place_object(obj, u.ux, u.uy);
				stackobj(obj);
				break;
			    }
#endif /* 0 */
			    /* right into your inventory */
			    You("snatch %s %s!", s_suffix(mon_nam(mtmp)),
				    xname(obj));
			    if (obj->otyp == CORPSE &&
				    touch_petrifies(&mons[obj->corpsenm]) &&
				    !uarmg && !Stone_resistance &&
				    !(poly_when_stoned(youmonst.data) &&
					polymon(PM_STONE_GOLEM))) {
				char kbuf[BUFSZ];

				Sprintf(kbuf, "%s尸体",
					an(mons[obj->corpsenm].mname));
				pline("去抢%s真是个能让你后悔莫及一辈子的错误。", kbuf);
				instapetrify(kbuf);
			    }
			    obj = hold_another_object(obj, "你把%s丢掉了！",
				    doname(obj), (const char *)0);
			    break;
			default:
			    /* to floor beneath mon */
			    You("把%s从%s的手里打了下来！", the(xname(obj)),
				    s_suffix(mon_nam(mtmp)));
			    if (obj->otyp == CRYSKNIFE &&
				    (!obj->oerodeproof || !rn2(10))) {
				obj->otyp = WORM_TOOTH;
				obj->oerodeproof = 0;
			    }
			    place_object(obj, mtmp->mx, mtmp->my);
			    stackobj(obj);
			    break;
		    }
		} else if (mtmp->mcanmove && !mtmp->msleeping)
		    pline("%s躲开了你的攻击。", Monnam(mtmp));
		else
		    You("fail to dislodge %s %s.", s_suffix(mon_nam(mtmp)),
			    xname(obj));
		wakeup(mtmp);
		if (!mtmp->mcanmove && !rn2(10)) {
		    mtmp->mcanmove = 1;
		    mtmp->mfrozen = 0;
		}
		break;
	    case T_DAZZLE:
	    	/* Short range stun attack */
	    	if (Blind) {
	    		You("啥也看不见！");
	    		return(0);
	    	}
	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			You("看不见自己了！");
			return(0);
		}
		for(i = 0; (i  <= ((techlev(tech_no) / 8) + 1) 
			&& isok(u.ux + (i*u.dx), u.uy + (i*u.dy))); i++) {
		    mtmp = m_at(u.ux + (i*u.dx), u.uy + (i*u.dy));
		    if (mtmp && canseemon(mtmp)) break;
		}
		if (!mtmp || !canseemon(mtmp)) {
			You("没法把眼神聚焦到任何物件上！");
			return (0);
		}
                You("死死地盯着%s。", mon_nam(mtmp));
                if (!haseyes(mtmp->data))
                	pline("……但是%s没有眼睛！", mon_nam(mtmp));
                else if (!mtmp->mcansee)
                	pline("……但是%s看不见你！", mon_nam(mtmp));
                if ((rn2(6) + rn2(6) + (techlev(tech_no) - mtmp->m_lev)) > 10) {
			You("dazzle %s!", mon_nam(mtmp));
			mtmp->mcanmove = 0;
			mtmp->mfrozen = rnd(10);
		} else {
                       pline("%s打破了你们之间的视线交汇！", Monnam(mtmp));
		}
               	t_timeout = rn1(50,25);
	    	break;
	    case T_BLITZ:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("没法在拿着武器的时候这么做！");
	    		return(0);
	    	} else if (uarms) {
			You("没法在拿着盾牌的时候这么做！");
	    		return(0);
	    	}
	    	if (!doblitz()) return (0);		
		
                t_timeout = rn1(1000,500);
	    	break;
            case T_PUMMEL:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("没法在拿着武器的时候这么做！");
	    		return(0);
	    	} else if (uarms) {
			You("没法在拿着盾牌的时候这么做！");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("鼓了鼓你的肌肉。");
			return(0);
		}
            	if (!blitz_pummel()) return(0);
                t_timeout = rn1(1000,500);
		break;
            case T_G_SLAM:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("没法在拿着武器的时候这么做！");
	    		return(0);
	    	} else if (uarms) {
			You("没法在拿着盾牌的时候这么做！");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("鼓了鼓你的肌肉。");
			return(0);
		}
            	if (!blitz_g_slam()) return(0);
                t_timeout = rn1(1000,500);
		break;
            case T_DASH:
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("stretch.");
			return(0);
		}
            	if (!blitz_dash()) return(0);
                t_timeout = rn1(50, 25);
		break;
            case T_POWER_SURGE:
            	if (!blitz_power_surge()) return(0);
		t_timeout = rn1(1000,500);
		break;            	
            case T_SPIRIT_BOMB:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("没法在拿着武器的时候这么做！");
	    		return(0);
	    	} else if (uarms) {
			You("没法在拿着盾牌的时候这么做！");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
            	if (!blitz_spirit_bomb()) return(0);
		t_timeout = rn1(1000,500);
		break;            	
	    case T_DRAW_BLOOD:
		if (!maybe_polyd(is_vampire(youmonst.data),
		  Race_if(PM_VAMPIRE))) {
		    /* ALI
		     * Otherwise we get problems with what we create:
		     * potions of vampire blood would no longer be
		     * appropriate.
		     */
		    You("必须处于自己的原始形态才能抽血。");
		    return(0);
		}
		obj = use_medical_kit(PHIAL, TRUE, "draw blood with");
		if (!obj)
		    return 0;
		if (u.ulevel <= 1) {
		    You_cant("找到瓶子。");
		    return 0;
		}
		check_unpaid(obj);
		if (obj->quan > 1L)
		    obj->quan--;
		else {
		    obj_extract_self(obj);
		    obfree(obj, (struct obj *)0);
		}
		pline("你用医疗包的工具抽了自己一小瓶的血。");
		losexp("抽血", TRUE);
		if (u.uexp > 0)
		    u.uexp = newuexp(u.ulevel - 1);
		otmp = mksobj(POT_VAMPIRE_BLOOD, FALSE, FALSE);
		otmp->cursed = obj->cursed;
		otmp->blessed = obj->blessed;
		(void) hold_another_object(otmp,
			"You fill, but have to drop, %s!", doname(otmp),
			(const char *)0);
		t_timeout = rn1(1000, 500);
		break;
	    default:
	    	pline ("错误！无%i效果", tech_no);
		break;
        }
        if (!can_limitbreak())
	    techtout(tech_no) = (t_timeout * (100 - techlev(tech_no))/100);

	/*By default,  action should take a turn*/
	return(1);
}

/* Whether or not a tech is in use.
 * 0 if not in use, turns left if in use. Tech is done when techinuse == 1
 */
int
tech_inuse(tech_id)
int tech_id;
{
        int i;

        if (tech_id < 1 || tech_id > MAXTECH) {
                impossible ("invalid tech: %d", tech_id);
                return(0);
        }
        for (i = 0; i < MAXTECH; i++) {
                if (techid(i) == tech_id) {
                        return (techt_inuse(i));
                }
        }
	return (0);
}

void
tech_timeout()
{
	int i;
	
        for (i = 0; i < MAXTECH; i++) {
	    if (techid(i) == NO_TECH)
		continue;
	    if (techt_inuse(i)) {
	    	/* Check if technique is done */
	        if (!(--techt_inuse(i)))
	        switch (techid(i)) {
		    case T_EVISCERATE:
			You("收回了自己的爪子。");
			/* You're using bare hands now,  so new msg for next attack */
			unweapon=TRUE;
			/* Lose berserk status */
			repeat_hit = 0;
			break;
		    case T_BERSERK:
			The("蒙住你意识的那片红色消退了。");
			break;
		    case T_KIII:
			You("冷静下来了。");
			break;
		    case T_FLURRY:
			You("relax.");
			break;
		    case T_E_FIST:
			You("感觉这股力量消失了。");
			break;
		    case T_SIGIL_TEMPEST:
			pline_The("爆破魔印消失了。");
			break;
		    case T_SIGIL_CONTROL:
			pline_The("控制魔印消失了。");
			break;
		    case T_SIGIL_DISCHARGE:
			pline_The("释放魔印消失了。");
			break;
		    case T_RAGE:
			Your("怒气消了下去。");
			break;
		    case T_POWER_SURGE:
			pline_The("你身上那股超棒的力量消失了。");
			break;
		    case T_BLINK:
			You("感觉时间的流速恢复了正常。");
			break;
		    case T_CHI_STRIKE:
			You("感觉手中的力量消失了。");
			break;
		    case T_CHI_HEALING:
			You("感觉那股治愈之力消失了。");
			break;
	            default:
	            	break;
	        } else switch (techid(i)) {
	        /* During the technique */
		    case T_RAGE:
			/* Bleed but don't kill */
			if (u.uhpmax > 1) u.uhpmax--;
			if (u.uhp > 1) u.uhp--;
			break;
		    case T_POWER_SURGE:
			/* Bleed off power.  Can go to zero as 0 power is not fatal */
			if (u.uenmax > 1) u.uenmax--;
			if (u.uen > 0) u.uen--;
			break;
	            default:
	            	break;
	        }
	    } 

	    if (techtout(i) > 0) techtout(i)--;
        }
}

void
docalm()
{
	int i, tech, n = 0;

	for (i = 0; i < MAXTECH; i++) {
	    tech = techid(i);
	    if (tech != NO_TECH && techt_inuse(i)) {
		aborttech(tech);
		n++;
	    }
	}
	if (n)
	    You("冷静下来了。");
}

static void
hurtmon(mtmp, tmp)
struct monst *mtmp;
int tmp;
{
	mtmp->mhp -= tmp;
	if (mtmp->mhp < 1) killed (mtmp);
#ifdef SHOW_DMG
	else showdmg(tmp);
#endif
}

static const struct 	innate_tech *
role_tech()
{
	switch (Role_switch) {
		case PM_ARCHEOLOGIST:	return (arc_tech);
		case PM_BARBARIAN:	return (bar_tech);
		case PM_CAVEMAN:	return (cav_tech);
		case PM_FLAME_MAGE:	return (fla_tech);
		case PM_HEALER:		return (hea_tech);
		case PM_ICE_MAGE:	return (ice_tech);
		case PM_KNIGHT:		return (kni_tech);
		case PM_MONK: 		return (mon_tech);
		case PM_NECROMANCER:	return (nec_tech);
		case PM_PRIEST:		return (pri_tech);
		case PM_RANGER:		return (ran_tech);
		case PM_ROGUE:		return (rog_tech);
		case PM_SAMURAI:	return (sam_tech);
#ifdef TOURIST        
		case PM_TOURIST:	return (tou_tech);
#endif        
		case PM_UNDEAD_SLAYER:	return (und_tech);
		case PM_VALKYRIE:	return (val_tech);
		case PM_WIZARD:		return (wiz_tech);
#ifdef YEOMAN
		case PM_YEOMAN:		return (yeo_tech);
#endif
		default: 		return ((struct innate_tech *) 0);
	}
}

static const struct     innate_tech *
race_tech()
{
	switch (Race_switch) {
		case PM_DOPPELGANGER:	return (dop_tech);
#ifdef DWARF
		case PM_DWARF:		return (dwa_tech);
#endif
		case PM_ELF:
		case PM_DROW:		return (elf_tech);
		case PM_GNOME:		return (gno_tech);
		case PM_HOBBIT:		return (hob_tech);
		case PM_HUMAN_WEREWOLF:	return (lyc_tech);
		case PM_VAMPIRE:	return (vam_tech);
		default: 		return ((struct innate_tech *) 0);
	}
}

void
adjtech(oldlevel,newlevel)
int oldlevel, newlevel;
{
	const struct   innate_tech  
		*tech = role_tech(), *rtech = race_tech();
	long mask = FROMEXPER;

	while (tech || rtech) {
	    /* Have we finished with the tech lists? */
	    if (!tech || !tech->tech_id) {
	    	/* Try the race intrinsics */
	    	if (!rtech || !rtech->tech_id) break;
	    	tech = rtech;
	    	rtech = (struct innate_tech *) 0;
		mask = FROMRACE;
	    }
		
	    for(; tech->tech_id; tech++)
		if(oldlevel < tech->ulevel && newlevel >= tech->ulevel) {
		    if (tech->ulevel != 1 && !tech_known(tech->tech_id))
			You("学会了如何施展%s！",
			  tech_names[tech->tech_id]);
		    learntech(tech->tech_id, mask, tech->tech_lev);
		} else if (oldlevel >= tech->ulevel && newlevel < tech->ulevel
		    && tech->ulevel != 1) {
		    learntech(tech->tech_id, mask, -1);
		    if (!tech_known(tech->tech_id))
			You("失去了施展%s的能力！",
			  tech_names[tech->tech_id]);
		}
	}
}

int
mon_to_zombie(monnum)
int monnum;
{
	if ((&mons[monnum])->mlet == S_ZOMBIE) return monnum;  /* is already zombie */
	if ((&mons[monnum])->mlet == S_KOBOLD) return PM_KOBOLD_ZOMBIE;
	if ((&mons[monnum])->mlet == S_GNOME) return PM_GNOME_ZOMBIE;
	if (is_orc(&mons[monnum])) return PM_ORC_ZOMBIE;
	if (is_dwarf(&mons[monnum])) return PM_DWARF_ZOMBIE;
	if (is_elf(&mons[monnum])) return PM_ELF_ZOMBIE;
	if (is_human(&mons[monnum])) return PM_HUMAN_ZOMBIE;
	if (monnum == PM_ETTIN) return PM_ETTIN_ZOMBIE;
	if (is_giant(&mons[monnum])) return PM_GIANT_ZOMBIE;
	/* Is it humanoid? */
	if (!humanoid(&mons[monnum])) return (-1);
	/* Otherwise,  return a ghoul or ghast */
	if (!rn2(4)) return PM_GHAST;
	else return PM_GHOUL;
}


/*WAC tinker code*/
STATIC_PTR int
tinker()
{
	int chance;
	struct obj *otmp = uwep;


	if (delay) {    /* not if (delay++), so at end delay == 0 */
		delay++;
#if 0
		use_skill(P_TINKER, 1); /* Tinker skill */
#endif
		/*WAC a bit of practice so even if you're interrupted
		you won't be wasting your time ;B*/
		return(1); /* still busy */
	}

	if (!uwep)
		return (0);

	You("完成了你的物品升级。");
	chance = 5;
/*	chance += PSKILL(P_TINKER); */
	if (rnl(10) < chance) {		
		upgrade_obj(otmp);
	} else {
		/* object downgrade  - But for now,  nothing :) */
	}

	setuwep(otmp, FALSE);
	You("现在正拿着%s！", doname(otmp));
	return(0);
}

/*WAC  draw energy from surrounding objects */
STATIC_PTR int
draw_energy()
{
	int powbonus = 1;
	if (delay) {    /* not if (delay++), so at end delay == 0 */
		delay++;
		confdir();
		if(isok(u.ux + u.dx, u.uy + u.dy)) {
			switch((&levl[u.ux + u.dx][u.uy + u.dy])->typ) {
			    case ALTAR: /* Divine power */
			    	powbonus =  (u.uenmax > 28 ? u.uenmax / 4
			    			: 7);
				break;
			    case THRONE: /* Regal == pseudo divine */
			    	powbonus =  (u.uenmax > 36 ? u.uenmax / 6
			    			: 6);			    		 	
				break;
			    case CLOUD: /* Air */
			    case TREE: /* Earth */
			    case LAVAPOOL: /* Fire */
			    case ICE: /* Water - most ordered form */
			    	powbonus = 5;
				break;
			    case AIR:
			    case MOAT: /* Doesn't freeze */
			    case WATER:
			    	powbonus = 4;
				break;
			    case POOL: /* Can dry up */
			    	powbonus = 3;
				break;
			    case FOUNTAIN:
			    	powbonus = 2;
				break;
			    case SINK:  /* Cleansing water */
			    	if (!rn2(3)) powbonus = 2;
				break;
			    case TOILET: /* Water Power...but also waste! */
			    	if (rn2(100) < 50)
			    		powbonus = 2;
			    	else powbonus = -2;
				break;
			    case GRAVE:
			    	powbonus = -4;
				break;
			    default:
				break;
			}
		}
		u.uen += powbonus;
		if (u.uen > u.uenmax) {
			delay = 0;
			u.uen = u.uenmax;
		}
		if (u.uen < 1) u.uen = 0;
		flags.botl = 1;
		return(1); /* still busy */
	}
	You("成功从周边的环境中抽取能量。");
	return(0);
}

static const char 
	*Enter_Blitz = "输入地牢快打命令，按.结束输入。";

/* Keep commands that reference the same blitz together 
 * Keep the BLITZ_START before the BLITZ_CHAIN before the BLITZ_END
 */
static const struct blitz_tab blitzes[] = { 	
	{"左左下下右", 5, blitz_chi_strike, T_CHI_STRIKE, BLITZ_START},
	{"左左下下左下左", 7, blitz_chi_strike, T_CHI_STRIKE, BLITZ_START},
	{"右右",  2, blitz_dash, T_DASH, BLITZ_START},
	{"左左",  2, blitz_dash, T_DASH, BLITZ_START},
	{"上上右右下下左", 7, blitz_e_fist, T_E_FIST, BLITZ_START},
	{"上右上右右下下左下左", 10, blitz_e_fist, T_E_FIST, BLITZ_START},
	{"下下右右下下右右", 8, blitz_power_surge, T_POWER_SURGE, BLITZ_START},
	{"下右下右下右下右", 8, blitz_power_surge, T_POWER_SURGE, BLITZ_START},
	{"左右左", 3, blitz_pummel, T_PUMMEL, BLITZ_CHAIN},
	{"右左右", 3, blitz_pummel, T_PUMMEL, BLITZ_CHAIN},
	{"下下下下", 4, blitz_g_slam, T_G_SLAM, BLITZ_END},
	{"下上下上上下下下", 8, blitz_spirit_bomb, T_SPIRIT_BOMB, BLITZ_END},
	{"", 0, (void *)0, 0, BLITZ_END} /* Array terminator */
};

#define MAX_BLITZ 50
#define MIN_CHAIN 2
#define MAX_CHAIN 5

/* parse blitz input */
static int
doblitz()
{
	int i, j, dx, dy, bdone = 0, tech_no;
	char buf[BUFSZ];
	char *bp;
	int blitz_chain[MAX_CHAIN], blitz_num;
        
	tech_no = (get_tech_no(T_BLITZ));

	if (tech_no == -1) {
		return(0);
	}
	
	if (u.uen < 10) {
		You("过于虚弱，无法这么做！");
            	return(0);
	}

	bp = buf;
	
	if (!getdir((char *)0)) return(0);
	if (!u.dx && !u.dy) {
		return(0);
	}
	
	dx = u.dx;
	dy = u.dy;

	doblitzlist();

    	for (i= 0; i < MAX_BLITZ; i++) {
		if (!getdir(Enter_Blitz)) return(0); /* Get directional input */
    		if (!u.dx && !u.dy && !u.dz) break;
    		if (u.dx == -1) {
    			*(bp) = 'L';
    			bp++;
    		} else if (u.dx == 1) {
    			*(bp) = 'R';
    			bp++;
    		}
    		if (u.dy == -1) {
    			*(bp) = 'U';
    			bp++;
    		} else if (u.dy == 1) {
    			*(bp) = 'D';
    			bp++;
    		}
    		if (u.dz == -1) {
    			*(bp) = '>';
    			bp++;
    		} else if (u.dz == 1) {
    			*(bp) = '<';
    			bp++;
    		}
    	}
	*(bp) = '.';
	bp++;
	*(bp) = '\0';
	bp = buf;

	/* Point of no return - You've entered and terminated a blitz, so... */
    	u.uen -= 10;

    	/* parse input */
    	/* You can't put two of the same blitz in a row */
    	blitz_num = 0;
    	while(strncmp(bp, ".", 1)) {
	    bdone = 0;
	    for (j = 0; blitzes[j].blitz_len; j++) {
	    	if (blitz_num >= MAX_CHAIN || 
	    	    blitz_num >= (MIN_CHAIN + (techlev(tech_no) / 10)))
	    		break; /* Trying to chain too many blitz commands */
		else if (!strncmp(bp, blitzes[j].blitz_cmd, blitzes[j].blitz_len)) {
	    		/* Trying to chain in a command you don't know yet */
			if (!tech_known(blitzes[j].blitz_tech))
				break;
	    		if (blitz_num) {
				/* Check if trying to chain two of the exact same 
				 * commands in a row 
				 */
	    			if (j == blitz_chain[(blitz_num - 1)]) 
	    				break;
	    			/* Trying to chain after chain finishing command */
	    			if (blitzes[blitz_chain[(blitz_num - 1)]].blitz_type 
	    							== BLITZ_END)
	    				break;
	    			/* Trying to put a chain starter after starting
	    			 * a chain
	    			 * Note that it's OK to put two chain starters in a 
	    			 * row
	    			 */
	    			if ((blitzes[j].blitz_type == BLITZ_START) &&
	    			    (blitzes[blitz_chain[(blitz_num - 1)]].blitz_type 
	    							!= BLITZ_START))
	    				break;
	    		}
			bp += blitzes[j].blitz_len;
			blitz_chain[blitz_num] = j;
			blitz_num++;
			bdone = 1;
			break;
		}
	    }
	    if (!bdone) {
		You("手忙脚乱了！");
		return(1);
	    }
    	}
	for (i = 0; i < blitz_num; i++) {
	    u.dx = dx;
	    u.dy = dy;
	    if (!( (*blitzes[blitz_chain[i]].blitz_funct)() )) break;
	}
	
    	/* done */
	return(1);
}

static void
doblitzlist()
{
	winid tmpwin;
	int i, n;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

        Sprintf(buf, "%16s %10s %-17s", "左上就是键盘的左上方移动按钮", "上就是按移动的上键", "右上就是键盘的右上方移动按钮");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
        Sprintf(buf, "%16s %10s %-17s", "左就是向左移动的按钮", "", "[R = Right]");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
        Sprintf(buf, "%16s %10s %-17s", "左下就是键盘的左下方移动按钮", "下就是按往下的移动键", "右下就是键盘的右下方移动按钮");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);        

        Sprintf(buf, "%-30s %10s   %s", "Name", "Type", "Command");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

        for (i = 0; blitzes[i].blitz_len; i++) {
	    if (tech_known(blitzes[i].blitz_tech)) {
                Sprintf(buf, "%-30s %10s   %s",
                    (i && blitzes[i].blitz_tech == blitzes[(i-1)].blitz_tech ?
                    	"" : tech_names[blitzes[i].blitz_tech]), 
                    (blitzes[i].blitz_type == BLITZ_START ? 
                    	"starter" :
                    	(blitzes[i].blitz_type == BLITZ_CHAIN ? 
	                    	"chain" : 
	                    	(blitzes[i].blitz_type == BLITZ_END ? 
                    			"finisher" : "未知"))),
                    blitzes[i].blitz_cmd);

		add_menu(tmpwin, NO_GLYPH, &any,
                         0, 0, ATR_NONE, buf, MENU_UNSELECTED);
	    }
	}
        end_menu(tmpwin, "当前已知的地牢快打组合技");

	n = select_menu(tmpwin, PICK_NONE, &selected);
	destroy_nhwindow(tmpwin);
	return;
}

static int
blitz_chi_strike()
{
	int tech_no;
	
	tech_no = (get_tech_no(T_CHI_STRIKE));

	if (tech_no == -1) {
		return(0);
	}

	if (u.uen < 1) {
		You("过于虚弱，无法这么做！");
            	return(0);
	}
	You("感到手中有一股能量涌过！");
	techt_inuse(tech_no) = techlev(tech_no) + 4;
	return(1);
}

static int
blitz_e_fist()
{
	int tech_no;
	const char *str;
	
	tech_no = (get_tech_no(T_E_FIST));

	if (tech_no == -1) {
		return(0);
	}
	
	str = makeplural(body_part(HAND));
	You("用意志导引元素之力，然后灌注进你的%s里。", str);
	techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/3 + 1)) + d(1,4) + 2;
	return 1;
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_pummel()
{
	int i = 0, tech_no;
	struct monst *mtmp;
	tech_no = (get_tech_no(T_PUMMEL));

	if (tech_no == -1) {
		return(0);
	}

	You("连着打出了数不清的攻击！");

	if (u.uswallow)
	    mtmp = u.ustuck;
	else
	    mtmp = m_at(u.ux + u.dx, u.uy + u.dy);

	if (!mtmp) {
		You("什么也没打到。");
		return (0);
	}
	if (!attack(mtmp)) return (0);
	
	/* Perform the extra attacks
	 */
	for (i = 0; (i < 4); i++) {
	    if (rn2(70) > (techlev(tech_no) + 30)) break;

	    if (u.uswallow)
		mtmp = u.ustuck;
	    else
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);

	    if (!mtmp) return (1);
	    if (!attack(mtmp)) return (1);
	} 
	
	return(1);
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_g_slam()
{
	int tech_no, tmp, canhitmon, objenchant;
	struct monst *mtmp;
	struct trap *chasm;

	tech_no = (get_tech_no(T_G_SLAM));

	if (tech_no == -1) {
		return(0);
	}

	mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	if (!mtmp) {
		You("什么也没打到。");
		return (0);
	}
	if (!attack(mtmp)) return (0);

	/* Slam the monster into the ground */
	mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	if (!mtmp || u.uswallow) return(1);

	You("hurl %s downwards...", mon_nam(mtmp));
	if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) return(1);

	if (need_four(mtmp)) canhitmon = 4;
	else if (need_three(mtmp)) canhitmon = 3;
	else if (need_two(mtmp)) canhitmon = 2;
	else if (need_one(mtmp)) canhitmon = 1;
	else canhitmon = 0;
	if (Upolyd) {
	    if (hit_as_four(&youmonst))	objenchant = 4;
	    else if (hit_as_three(&youmonst)) objenchant = 3;
	    else if (hit_as_two(&youmonst)) objenchant = 2;
	    else if (hit_as_one(&youmonst)) objenchant = 1;
	    else if (need_four(&youmonst)) objenchant = 4;
	    else if (need_three(&youmonst)) objenchant = 3;
	    else if (need_two(&youmonst)) objenchant = 2;
	    else if (need_one(&youmonst)) objenchant = 1;
	    else objenchant = 0;
	} else
	    objenchant = u.ulevel / 4;

	tmp = (5 + rnd(6) + (techlev(tech_no) / 5));
	
	chasm = maketrap(u.ux + u.dx, u.uy + u.dy, PIT);
	if (chasm) {
	    if (!is_flyer(mtmp->data) && !is_clinger(mtmp->data))
		mtmp->mtrapped = 1;
	    chasm->tseen = 1;
	    levl[u.ux + u.dx][u.uy + u.dy].doormask = 0;
	    pline("%s slams into the ground, creating a crater!", Monnam(mtmp));
	    tmp *= 2;
	}

	mselftouch(mtmp, "Falling, ", TRUE);
	if (!DEADMONSTER(mtmp)) {
	    if (objenchant < canhitmon)
		pline("%s看起来没有受伤。", Monnam(mtmp));
	    else if ((mtmp->mhp -= tmp) <= 0) {
		if(!cansee(u.ux + u.dx, u.uy + u.dy))
		    pline("它被摧毁了！");
		else {
		    You("destroy %s!", 	
		    	mtmp->mtame
			    ? x_monnam(mtmp, ARTICLE_THE, "poor",
				mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE)
			    : mon_nam(mtmp));
		}
		xkilled(mtmp,0);
	    }
	}

	return(1);
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_dash()
{
	int tech_no;
	tech_no = (get_tech_no(T_DASH));

	if (tech_no == -1) {
		return(0);
	}
	
	if ((!Punished || carried(uball)) && !u.utrap)
	    You("向前猛冲！");
	hurtle(u.dx, u.dy, 2, FALSE);
	multi = 0;		/* No paralysis with dash */
	return 1;
}

static int
blitz_power_surge()
{
	int tech_no, num;
	
	tech_no = (get_tech_no(T_POWER_SURGE));

	if (tech_no == -1) {
		return(0);
	}

	if (Upolyd) {
		You("没法在当前的生物形态中发挥出全部潜力。");
		return(0);
	}
    	You("释放出了自身能量的全部潜力！");
	num = 50 + (2 * techlev(tech_no));
    	techt_inuse(tech_no) = num + 1;
	u.uenmax += num;
	u.uen = u.uenmax;
	return 1;
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_spirit_bomb()
{
	int tech_no, num;
	int sx = u.ux, sy = u.uy, i;
	
	tech_no = (get_tech_no(T_SPIRIT_BOMB));

	if (tech_no == -1) {
		return(0);
	}

	You("聚焦自己的能量……");
	
	if (u.uen < 10) {
		pline("但是这股能量滋滋了几下就没影了。");
		u.uen = 0;
	}

	num = 10 + (techlev(tech_no) / 5);
	num = (u.uen < num ? u.uen : num);
	
	u.uen -= num;
	
	for( i = 0; i < 2; i++) {		
	    if (!isok(sx,sy) || !cansee(sx,sy) || 
	    		IS_STWALL(levl[sx][sy].typ) || u.uswallow)
	    	break;

	    /* Display the center of the explosion */
	    tmp_at(DISP_FLASH, explosion_to_glyph(EXPL_MAGICAL, S_explode5));
	    tmp_at(sx, sy);
	    delay_output();
	    tmp_at(DISP_END, 0);

	    sx += u.dx;
	    sy += u.dy;
	}
	/* Magical Explosion */
	explode(sx, sy, 10, (d(3,6) + num), WAND_CLASS, EXPL_MAGICAL);
	return 1;
}

#ifdef DEBUG
void
wiz_debug_cmd() /* in this case, allow controlled loss of techniques */
{
	int tech_no, id, n = 0;
	long mask;
	if (gettech(&tech_no)) {
		id = techid(tech_no);
		if (id == NO_TECH) {
		    impossible("未知技巧%d？", tech_no);
		    return;
		}
		mask = tech_list[tech_no].t_intrinsic;
		if (mask & FROMOUTSIDE) n++;
		if (mask & FROMRACE) n++;
		if (mask & FROMEXPER) n++;
		if (!n) {
		    impossible("No intrinsic masks set (0x%lX).", mask);
		    return;
		}
		n = rn2(n);
		if (mask & FROMOUTSIDE && !n--) mask = FROMOUTSIDE;
		if (mask & FROMRACE && !n--) mask = FROMRACE;
		if (mask & FROMEXPER && !n--) mask = FROMEXPER;
		learntech(id, mask, -1);
		if (!tech_known(id))
		    You("失去了施展%s的能力。", tech_names[id]);
	}
}
#endif /* DEBUG */
