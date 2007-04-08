/*
 * psiiconset.cpp - the Psi iconset class
 * Copyright (C) 2001-2003  Justin Karneges, Michail Pishchagin
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

#include "psiiconset.h"
#include "psievent.h"
#include "common.h"
#include "userlist.h"
#include "anim.h"
#include "im.h"

#include <qfileinfo.h>
#include <q3dict.h>
#include <q3ptrlist.h>

using namespace XMPP;

//----------------------------------------------------------------------------
// PsiIconset
//----------------------------------------------------------------------------

class PsiIconset::Private
{
private:
	PsiIconset *psi;
public:
	Iconset system;

	Private(PsiIconset *_psi) {
		psi = _psi;
		psi->emoticons.setAutoDelete(true);
		psi->roster.setAutoDelete(true);
	}

	QString iconsetPath(QString name) {
		QStringList dirs;
		dirs << ".";
		dirs << g.pathHome;
		dirs << g.pathBase;

		QStringList::Iterator it = dirs.begin();
		for ( ; it != dirs.end(); ++it) {
			QString fileName = *it + "/iconsets/" + name;

			QFileInfo fi(fileName);
			if ( fi.exists() )
				return fileName;
		}

		return QString::null;
	}

	void stripFirstAnimFrame(Iconset &is) {
		QListIterator<Icon*> it = is.iterator();
		while (it.hasNext()) {
			it.next()->stripFirstAnimFrame();
		}
	}

	void loadIconset(Iconset *to, Iconset *from) {
		if ( !to ) {
			qWarning("PsiIconset::loadIconset(): 'to' iconset is NULL!");
			if ( from )
				qWarning("from->name() = '%s'", from->name().latin1());
			return;
		}
		if ( !from ) {
			qWarning("PsiIconset::loadIconset(): 'from' iconset is NULL!");
			if ( to )
				qWarning("to->name() = '%s'", to->name().latin1());
			return;
		}

		QListIterator<Icon*> it = from->iterator();
		while( it.hasNext()) {
			Icon *icon = it.next();

			if ( icon && !icon->name().isEmpty() ) {
				Icon *toIcon = (Icon *)to->icon(icon->name());
				if ( toIcon ) {
					if ( icon->anim() ) {
						toIcon->setAnim  ( *icon->anim(), false );
						toIcon->setImpix ( icon->impix(), false );
					}
					else {
						toIcon->setAnim  ( Anim(),        false );
						toIcon->setImpix ( icon->impix(), false );
					}
				}
				else
					to->setIcon( icon->name(), *icon );
			}
		}
		
		to->setInformation(*from);
	}

	Icon *jid2icon(const Jid &jid, const QString &iconName)
	{
		// first level -- global default icon
		Icon *icon = (Icon *)IconsetFactory::iconPtr(iconName);

		// second level -- transport icon
		if ( jid.user().isEmpty() || option.useTransportIconsForContacts ) {
			QMap<QString, QRegExp> services;
			services["aim"]		= QRegExp("^aim");
			services["gadugadu"]	= QRegExp("^gg");
			services["icq"]		= QRegExp("^icq");
			services["msn"]		= QRegExp("^msn");
			services["yahoo"]	= QRegExp("^yahoo");
			services["sms"]		= QRegExp("^sms");

			bool found = false;

			QMap<QString, QRegExp>::Iterator it = services.begin();
			for ( ; it != services.end(); ++it) {
				QRegExp rx = it.data();
				if ( rx.search(jid.host()) != -1 ) {
					// get the iconset name of the current service
					QMap<QString, QString>::Iterator it2 = option.serviceRosterIconset.find(it.key());
					if ( it2 != option.serviceRosterIconset.end() ) {
						Iconset *is = psi->roster.find(it2.data());
						if ( is ) {
							Icon *i = (Icon *)is->icon(iconName);
							if ( i ) {
								icon = i;
								found = true;
								break;
							}
						}
					}
				}
			}

			// let's try the default transport iconset then...
			if ( !found && jid.user().isEmpty() ) {
				Iconset *is = psi->roster.find(option.serviceRosterIconset["transport"]);
				if ( is ) {
					Icon *i = (Icon *)is->icon(iconName);
					if ( i )
						icon = i;
				}
			}
		}

		// third level -- custom icons
		QMap<QString, QString>::Iterator it = option.customRosterIconset.begin();
		for ( ; it != option.customRosterIconset.end(); ++it) {
			QRegExp rx = QRegExp(it.key());
			if ( rx.search(jid.userHost()) != -1 ) {
				Iconset *is = psi->roster.find(it.data());
				if ( is ) {
					Icon *i = (Icon *)is->icon(iconName);
					if ( i )
						icon = (Icon *)is->icon(iconName);
				}
			}
		}

		return icon;
	}

	Iconset systemIconset(bool *ok)
	{
		Iconset def;
		*ok = def.load(":/iconsets/system/default");

		if ( option.systemIconset != "default" ) {
			Iconset is;
			is.load ( iconsetPath("system/" + option.systemIconset) );

			loadIconset(&def, &is);
		}

		stripFirstAnimFrame( def );

		return def;
	}

	Iconset *defaultRosterIconset(bool *ok)
	{
		Iconset *def = new Iconset;
		*ok = def->load (":/iconsets/roster/default");

		if ( option.defaultRosterIconset != "default" ) {
			Iconset is;
			is.load ( iconsetPath("roster/" + option.defaultRosterIconset) );

			loadIconset(def, &is);
		}

		stripFirstAnimFrame( *def );

		return def;
	}

	Q3PtrList<Iconset> emoticons()
	{
		Q3PtrList<Iconset> emo;

		foreach(QString name, option.emoticons) {
			Iconset *is = new Iconset;
			if ( is->load ( iconsetPath("emoticons/" + name) ) ) {
				PsiIconset::removeAnimation(is);
				is->addToFactory();
				emo.append( is );
			}
			else
				delete is;
		}

		return emo;
	}
};

PsiIconset::PsiIconset()
{
	d = new Private(this);
}

PsiIconset::~PsiIconset()
{
	delete d;
}

bool PsiIconset::loadSystem()
{
	bool ok;
	Iconset sys = d->systemIconset(&ok);
	d->loadIconset( &d->system, &sys );

	//d->system = d->systemIconset();
	d->system.addToFactory();

	return ok;
}

bool PsiIconset::loadAll()
{
	if ( !loadSystem() )
		return false;

	bool ok;

	// load roster
	roster.clear();

	// default roster iconset
	Iconset *def = d->defaultRosterIconset(&ok);
	def->addToFactory();
	roster.insert (option.defaultRosterIconset, def);

	// load only necessary roster iconsets
	QStringList rosterIconsets;

	QMap<QString, QString>::Iterator it = option.serviceRosterIconset.begin();
	for ( ; it != option.serviceRosterIconset.end(); ++it)
		if ( rosterIconsets.findIndex( it.data() ) == -1 )
			rosterIconsets << it.data();

	it = option.customRosterIconset.begin();
	for ( ; it != option.customRosterIconset.end(); ++it)
		if ( rosterIconsets.findIndex( it.data() ) == -1 )
			rosterIconsets << it.data();

	QStringList::Iterator it2 = rosterIconsets.begin();
	for ( ; it2 != rosterIconsets.end(); ++it2) {
		if ( *it2 == option.defaultRosterIconset )
			continue;

		Iconset *is = new Iconset;
		if ( is->load (d->iconsetPath("roster/" + *it2)) ) {
			is->addToFactory ();
			d->stripFirstAnimFrame( *is );
			roster.insert (*it2, is);
		}
		else
		     delete is;
	}

	// load emoticons
	emoticons.clear();
	emoticons = d->emoticons();

	return ok;
}

bool PsiIconset::optionsChanged(const Options *old)
{
	bool ok = loadSystem();

	// default roster iconset
	if ( old->defaultRosterIconset != option.defaultRosterIconset ) {
		Iconset *newDef = d->defaultRosterIconset(&ok);
		Iconset *oldDef = roster[old->defaultRosterIconset];
		d->loadIconset( oldDef, newDef );

		roster.setAutoDelete(false);
		roster.remove(old->defaultRosterIconset);
		roster.setAutoDelete(true);

		roster.insert (option.defaultRosterIconset, oldDef);
		delete newDef;
	}

	// service&custom roster iconsets
	if (  operator!=(old->serviceRosterIconset,option.serviceRosterIconset) || operator!=(old->customRosterIconset,option.customRosterIconset)) {
		QStringList rosterIconsets;

		QMap<QString, QString>::Iterator it = option.serviceRosterIconset.begin();
		for ( ; it != option.serviceRosterIconset.end(); ++it)
			if ( rosterIconsets.findIndex( it.data() ) == -1 )
				rosterIconsets << it.data();

		it = option.customRosterIconset.begin();
		for ( ; it != option.customRosterIconset.end(); ++it)
			if ( rosterIconsets.findIndex( it.data() ) == -1 )
				rosterIconsets << it.data();

		QStringList::Iterator it2 = rosterIconsets.begin();
		for ( ; it2 != rosterIconsets.end(); ++it2) {
			if ( *it2 == option.defaultRosterIconset )
				continue;

			Iconset *is = new Iconset;
			if ( is->load (d->iconsetPath("roster/" + *it2)) ) {
				d->stripFirstAnimFrame( *is );
				Iconset *oldis = roster[*it2];

				if ( oldis )
					d->loadIconset( oldis, is );
				else {
					is->addToFactory ();
					roster.insert (*it2, is);
				}
			}
			else
				delete is;
		}

		bool clear = false;
		while ( !clear ) {
			clear = true;

			Q3DictIterator<Iconset> it3 ( roster );
			for ( ; it3.current(); ++it3) {
				QString name = it3.currentKey();
				if ( name == option.defaultRosterIconset )
					continue;

				it2 = rosterIconsets.find( name );
				if ( it2 == rosterIconsets.end() ) {
					// remove redundant iconset
					roster.remove( name );
					clear = false;
					break;
				}
			}
		}
	}

	// load emoticons
	if ( old->emoticons != option.emoticons ) {
		emoticons.clear();
		emoticons = d->emoticons();
	}

	return old->defaultRosterIconset != option.defaultRosterIconset;
}

Icon *PsiIconset::event2icon(PsiEvent *e)
{
	QString icon;
	if(e->type() == PsiEvent::Message) {
		MessageEvent *me = (MessageEvent *)e;
		const Message &m = me->message();
		if(m.type() == "headline")
			icon = "psi/headline";
		else if(m.type() == "chat")
			icon = "psi/chat";
		else if(m.type() == "error")
			icon = "psi/system";
		else
			icon = "psi/message";
	}
	else if(e->type() == PsiEvent::File) {
		icon = "psi/file";
	}
	else {
		icon = "psi/system";
	}

	return d->jid2icon(e->from(), icon);
}

static QString status2name(int s)
{
	QString name;
	switch ( s ) {
	case STATUS_OFFLINE:
		name = "status/offline";
		break;
	case STATUS_AWAY:
		name = "status/away";
		break;
	case STATUS_XA:
		name = "status/xa";
		break;
	case STATUS_DND:
		name = "status/dnd";
		break;
	case STATUS_INVISIBLE:
		name = "status/invisible";
		break;
	case STATUS_CHAT:
		name = "status/chat";
		break;

	case STATUS_ASK:
		name = "status/ask";
		break;
	case STATUS_NOAUTH:
		name = "status/noauth";
		break;
	case STATUS_ERROR:
		name = "status/error";
		break;

	case -1:
		name = "psi/connect";
		break;

	case STATUS_ONLINE:
	default:
		name = "status/online";
	}

	return name;
}

Icon *PsiIconset::statusPtr(int s)
{
	return (Icon *)IconsetFactory::iconPtr(status2name(s));
}

Icon PsiIconset::status(int s)
{
	Icon *icon = statusPtr(s);
	if ( icon )
		return *icon;
	return Icon();
}

Icon *PsiIconset::statusPtr(const XMPP::Status &s)
{
	return statusPtr(makeSTATUS(s));
}

Icon PsiIconset::status(const XMPP::Status &s)
{
	return status(makeSTATUS(s));
}

Icon *PsiIconset::transportStatusPtr(QString name, int s)
{
	Icon *icon = 0;

	QMap<QString, QString>::Iterator it = option.serviceRosterIconset.begin();
	for ( ; it != option.serviceRosterIconset.end(); ++it) {
		if (name == it.key()) {
			Iconset *is = roster.find(it.data());
			if ( is ) {
				icon = (Icon *)is->icon(status2name(s));
				if ( icon )
					break;
			}
		}
	}

	if ( !icon )
		icon = statusPtr(s);

	return icon;
}

Icon *PsiIconset::transportStatusPtr(QString name, const XMPP::Status &s)
{
	return transportStatusPtr(name, makeSTATUS(s));
}

Icon PsiIconset::transportStatus(QString name, int s)
{
	Icon *icon = transportStatusPtr(name, s);
	if ( icon )
		return *icon;
	return Icon();
}

Icon PsiIconset::transportStatus(QString name, const XMPP::Status &s)
{
	Icon *icon = transportStatusPtr(name, s);
	if ( icon )
		return *icon;
	return Icon();
}

Icon *PsiIconset::statusPtr(const XMPP::Jid &jid, int s)
{
	return d->jid2icon(jid, status2name(s));
}

Icon *PsiIconset::statusPtr(const XMPP::Jid &jid, const XMPP::Status &s)
{
	return statusPtr(jid, makeSTATUS(s));
}

Icon PsiIconset::status(const XMPP::Jid &jid, int s)
{
	Icon *icon = statusPtr(jid, s);
	if ( icon )
		return *icon;
	return Icon();
}

Icon PsiIconset::status(const XMPP::Jid &jid, const XMPP::Status &s)
{
	Icon *icon = statusPtr(jid, s);
	if ( icon )
		return *icon;
	return Icon();
}

Icon *PsiIconset::statusPtr(UserListItem *u)
{
	if ( !u )
		return 0;

	int s = 0;
	if ( !u->presenceError().isEmpty() )
		s = STATUS_ERROR;
	else if ( u->isTransport() ) {
		if ( u->isAvailable() )
			s = makeSTATUS( (*(u->priority())).status() );
		else
			s = STATUS_OFFLINE;
	}
	else if ( u->ask() == "subscribe" && !u->isAvailable() && !u->isTransport() )
		s = STATUS_ASK;
	else if ( (u->subscription().type() == Subscription::From || u->subscription().type() == Subscription::None) && !u->isAvailable() && !u->isPrivate() )
		s = STATUS_NOAUTH;
	else if( !u->isAvailable() )
		s = STATUS_OFFLINE;
	else
		s = makeSTATUS( (*(u->priority())).status() );

	return statusPtr(u->jid(), s);
}

Icon PsiIconset::status(UserListItem *u)
{
	Icon *icon = statusPtr(u);
	if ( icon )
		return *icon;
	return Icon();
}

const Iconset &PsiIconset::system() const
{
	return d->system;
}

void PsiIconset::stripFirstAnimFrame(Iconset *is)
{
	if ( is )
		d->stripFirstAnimFrame(*is);
}

void PsiIconset::removeAnimation(Iconset *is)
{
	if ( is ) {
		QListIterator<Icon*> it = is->iterator();
		while (it.hasNext()) {
			it.next()->removeAnim(false);
		}
	}
}