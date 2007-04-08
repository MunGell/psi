/*
 * common.h - contains all the common variables and functions for Psi
 * Copyright (C) 2001-2003  Justin Karneges
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QStringList>
#include <QWidget>
#include <QMessageBox>
#include <QKeySequence>
#include <QList>
#include "im.h"
#include "varlist.h"
#include "psiiconset.h"
#include "statuspreset.h"

#define PROXY_NONE       0
#define PROXY_HTTPS      1
#define PROXY_SOCKS4     2
#define PROXY_SOCKS5     3

#define STATUS_OFFLINE   0
#define STATUS_ONLINE    1
#define STATUS_AWAY      2
#define STATUS_XA        3
#define STATUS_DND       4
#define STATUS_INVISIBLE 5
#define STATUS_CHAT      6

#define STATUS_ASK	 100
#define STATUS_NOAUTH	 101
#define STATUS_ERROR	 102

// global
struct PsiGlobal
{
	QString pathBase, pathHome, pathProfiles;
};

extern QString activeProfile;

//extern QStringList dtcp_hostList;
//extern int dtcp_port;
//extern QString dtcp_proxy;
//extern bool link_test;
extern Qt::WFlags psi_dialog_flags;

// options stuff
enum { 
	cOnline, 
	cOffline, 
	cAway, 
	cDND, 
	cProfileFore, 
	cProfileBack, 
	cGroupFore, 
	cGroupBack, 
	cListBack, 
	cAnimFront, 
	cAnimBack,
	cStatus,
	cNumColors // A guard to store the number of colors
};
enum { fRoster, fMessage, fChat, fPopup };
enum { eMessage, eChat1, eChat2, eHeadline, eSystem, eOnline, eOffline, eSend, eIncomingFT, eFTComplete };
enum { dcClose, dcHour, dcDay, dcNever };

struct Options
{
	QColor color[cNumColors];
	QString font[4];
	int smallFontSize; // do not modify or save/load this value! it is calculated at run time!
	int alertStyle;

	QString systemIconset;
	QStringList emoticons;
	QString defaultRosterIconset;
	QMap<QString, QString> serviceRosterIconset;
	QMap<QString, QString> customRosterIconset;
	int rosterOpacity;

	bool useleft, singleclick, hideMenubar, askOnline, askOffline, popupMsgs, popupChats, popupHeadlines, popupFiles, raise;
	bool alwaysOnTop, noAwaySound, noAwayPopup, noUnlistedPopup, rosterAnim, autoVCardOnLogin, xmlConsoleOnLogin;
	bool useDock, dockDCstyle, dockHideMW, dockToolMW, isWMDock;
	bool smallChats, chatLineEdit, useTabs, putTabsAtBottom, usePerTabCloseButton, autoRosterSize, autoRosterSizeGrowTop, autoResolveNicksOnAdd, brushedMetal;

	bool autoCopy; // although this setting is ignored under linux,
	               // it is preserved in case user uses the same config file on different platforms
	bool useCaps;

	bool oldSmallChats; //Filthy hack, see chat code.
	int delChats, browser;
	
	bool useRC;

	int defaultAction;
	int incomingAs;
	QStringList recentStatus; //recent status messages
	QMap<QString,StatusPreset> sp; // Status message presets.

	int asAway, asXa, asOffline;
	bool use_asAway, use_asXa, use_asOffline;
	QString asMessage;
	QString onevent[10];

	// Added by Kiko 020621: points to the directory where the trusted
	// certificates used in validating the server's certificate are kept
	QString trustCertStoreDir;

	QString player;
	QString customBrowser, customMailer;

	bool ignoreHeadline, ignoreNonRoster, excludeGroupChatsFromIgnore, scrollTo, keepSizes, useEmoticons, alertOpenChats;
	bool raiseChatWindow, showSubjects, showCounter, chatSays, chatSoftReturn, showGroupCounts;

	QSize sizeEventDlg, sizeChatDlg, sizeTabDlg;
	bool jidComplete, grabUrls, noGCSound;

	struct ToolbarPrefs {
		bool dirty;

		QString name;
		bool on;
		bool locked;
		bool stretchable;
		QStringList keys;

		Qt::Dock dock;
		int index;
		bool nl;
		int extraOffset;
	};
	QMap< QString, QList<ToolbarPrefs> > toolbars;

	// groupchat highlighting/nick colouring
	bool gcHighlighting, gcNickColoring;
	QStringList gcHighlights, gcNickColors;

	bool clNewHeadings;
	bool outlineHeadings;

	// passive popups
	bool ppIsOn;
	bool ppMessage, ppHeadline, ppChat, ppOnline, ppStatus, ppOffline, ppFile;
	int  ppJidClip, ppStatusClip, ppTextClip, ppHideTime;
	QColor ppBorderColor;

	// Bouncing of the dock (Mac OS X)
	typedef enum { NoBounce, BounceOnce, BounceForever } BounceDockSetting;
	BounceDockSetting bounceDock;

	struct {
		bool roster, services;
	} lockdown;

	bool useTransportIconsForContacts;

	// roster sorting styles
	typedef enum Roster_ContactSortStyle {
		ContactSortStyle_Status = 0,
		ContactSortStyle_Alpha
	};
	Roster_ContactSortStyle rosterContactSortStyle;

	typedef enum Roster_GroupSortStyle {
		GroupSortStyle_Alpha = 0,
		GroupSortStyle_Rank
	};
	Roster_GroupSortStyle rosterGroupSortStyle;

	typedef enum Roster_AccountSortStyle {
		AccountSortStyle_Alpha = 0,
		AccountSortStyle_Rank
	};
	Roster_AccountSortStyle rosterAccountSortStyle;

	bool showTips;
	int tipNum;

	bool discoItems, discoInfo;

	bool autoAuth, notifyAuth;

	// event priority
	enum { EventPriorityDontCare = -1 };
	int eventPriorityHeadline;
	int eventPriorityChat;
	int eventPriorityMessage;
	int eventPriorityAuth;
	int eventPriorityFile;
	int eventPriorityRosterExchange;

	// Message events
	bool messageEvents;
	bool inactiveEvents;

	int dtPort;
	QString dtExternal;

	// Last used path remembering
	QString lastPath;
	QString lastSavePath;

	//background images
	QString chatBgImage, rosterBgImage;

	// Global accelerators
	QList<QKeySequence> globalAccels;
};

// functions

QString getResourcesDir();
QString getHomeDir();

QString getHistoryDir();
QString getVCardDir();

QString qstrlower(QString);
int qstrcasecmp(const QString &str1, const QString &str2);
int qstringlistmatch(QStringList &list, const QString &str);
QString qstringlistlookup(QStringList &list, int x);
QString CAP(const QString &str);

QString status2txt(int status);

QString qstrquote(const QString &, int width=60, bool quoteEmpty=FALSE);
QString plain2rich(const QString &);
QString rich2plain(const QString &);
QString clipStatus(const QString &str, int width, int height);
QString resolveEntities(const QString &);
QString linkify(const QString &);
QString emoticonify(const QString &in);

QString encodePassword(const QString &, const QString &);
QString decodePassword(const QString &, const QString &);

Icon category2icon(const QString &category, const QString &type, int status=STATUS_ONLINE);

void bringToFront(QWidget *w, bool grabFocus = true);
int hexChar2int(char c);
char int2hexChar(int x);

QString logencode(QString);
QString logdecode(const QString &);

bool operator!=(const QMap<QString, QString> &, const QMap<QString, QString> &);
void qstringlistisort(QStringList &c);

void openURL(const QString &);

QString getOSName();
int getTZOffset();
QString getTZString();

bool fileCopy(const QString &src, const QString &dest);

#ifdef Q_WS_MAC
void stopDockTileBounce();
void bounceDockTile();
#endif

#ifdef Q_WS_X11
void x11wmClass(Display *dsp, WId wid, QString resName);
#define X11WM_CLASS(x)	x11wmClass(x11Display(), winId(), (x));
#else
#define X11WM_CLASS(x)	/* dummy */
#endif

void soundPlay(const QString &);

XMPP::Status makeStatus(int, const QString &);
XMPP::Status makeStatus(int, const QString &, int);
int makeSTATUS(const XMPP::Status &);

void replaceWidget(QWidget *, QWidget *);
void closeDialogs(QWidget *);

QString enc822jid(const QString &);
QString dec822jid(const QString &);

extern
QString PROG_NAME, PROG_VERSION, PROG_CAPS_NODE, PROG_CAPS_VERSION, PROG_OPTIONS_NS, PROG_STORAGE_NS;

extern
Options option;

extern
PsiGlobal g;

extern
PsiIconset *is;

extern
bool useSound;

// helper class for toolbars
class StretchWidget: public QWidget
{
public:
	StretchWidget(QWidget *parent)
	: QWidget( parent )
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}
};

typedef QList<XMPP::Jid> JidList;
typedef QListIterator<XMPP::Jid> JidListIt;


#endif