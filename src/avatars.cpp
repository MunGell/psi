/*
 * avatars.cpp 
 * Copyright (C) 2006  Remko Troncon
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

/*
 * TODO:
 * - Be more efficient about storing avatars in memory
 * - Move ovotorChanged() to Avatar, and only listen to the active avatars
 *   being changed.
 */

#include <QDomElement>
#include <QtCrypto>
#include <QPixmap>
#include <QDateTime>
#include <QFile>
#include <QBuffer>

#include "xmpp.h"
#include "xmpp_xmlcommon.h"
#include "xmpp_vcard.h"
#include "avatars.h"
#include "common.h"
#include "psiaccount.h"
#include "profiles.h"
#include "vcardfactory.h"
#include "pepmanager.h"

#define MAX_AVATAR_SIZE 96
#define MAX_AVATAR_DISPLAY_SIZE 64

using namespace QCA;

//------------------------------------------------------------------------------

static QByteArray scaleAvatar(const QByteArray& b)	
{
	//int maxSize = (option.avatarsSize > MAX_AVATAR_SIZE ? MAX_AVATAR_SIZE : option.avatarsSize);
	int maxSize = MAX_AVATAR_SIZE;
	QImage i(b);
	if (i.isNull()) {
		qWarning("AvatarFactory::scaleAvatar(): Null image (unrecognized format?)");
		return QByteArray();
	}
	else if (i.width() > maxSize || i.height() > maxSize) {
		QImage image = i.scaled(maxSize,maxSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		image.save(&buffer, "PNG");
		return ba;
	}
	else {
		return b;
	}
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Avatar: Base class for avatars.
//------------------------------------------------------------------------------

Avatar::Avatar()
{
}


Avatar::~Avatar()
{
}

void Avatar::setImage(const QImage& i)
{
	if (i.width() > MAX_AVATAR_DISPLAY_SIZE || i.height() > MAX_AVATAR_DISPLAY_SIZE)
		pixmap_.convertFromImage(i.scaled(MAX_AVATAR_DISPLAY_SIZE,MAX_AVATAR_DISPLAY_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation));
	else
		pixmap_.convertFromImage(i);

}

void Avatar::setImage(const QByteArray& ba)
{
	setImage(QImage(ba));
}

void Avatar::setImage(const QPixmap& p)
{
	if (p.width() > MAX_AVATAR_DISPLAY_SIZE || p.height() > MAX_AVATAR_DISPLAY_SIZE)
		pixmap_ = p.scaled(MAX_AVATAR_DISPLAY_SIZE,MAX_AVATAR_DISPLAY_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
	else
		pixmap_ = p;
}

void Avatar::resetImage()
{
	pixmap_ = QPixmap();
}

//------------------------------------------------------------------------------
// CachedAvatar: Base class for avatars which are requested and are to be cached
//------------------------------------------------------------------------------

class CachedAvatar : public Avatar
{
public:
	CachedAvatar() { };
	virtual void updateHash(const QString& h);

protected:
	virtual const QString& hash() const { return hash_; }
	virtual void requestAvatar() { }
	virtual void avatarUpdated() { }
	
	virtual bool isCached(const QString& hash);
	virtual void loadFromCache(const QString& hash);
	virtual void saveToCache(const QString& hash, const QByteArray& data);

private:
	QString hash_;
};


void CachedAvatar::updateHash(const QString& h)
{
	if (hash_ != h) {
		hash_ = h;
		if (h.isEmpty()) {
			hash_ = "";
			resetImage();
			avatarUpdated();
		}
		else if (isCached(h)) {
			loadFromCache(h);
			avatarUpdated();
		}
		else {
			resetImage();
			avatarUpdated();
			requestAvatar();
		}
	}
}

bool CachedAvatar::isCached(const QString& h)
{
	return QDir(AvatarFactory::getCacheDir()).exists(h);
}

void CachedAvatar::loadFromCache(const QString& h)
{
	// printf("Loading avatar from cache\n");
	hash_ = h;
	setImage(QImage(QDir(AvatarFactory::getCacheDir()).filePath(h)));
	if (pixmap().isNull()) {
		qWarning("CachedAvatar::loadFromCache(): Null pixmap. Unsupported format ?");
	}
}

void CachedAvatar::saveToCache(const QString& hash, const QByteArray& data)
{
	// Write file to cache
	// printf("Saving %s to cache.\n",hash.latin1());
	QString fn = QDir(AvatarFactory::getCacheDir()).filePath(hash);
	QFile f(fn);
	if (f.open(IO_WriteOnly)) {
		f.writeBlock(data);
		f.close();
	}
	else
		printf("Error opening %s for writing.\n",f.name().latin1());
	
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  PEPAvatar: PEP Avatars
//------------------------------------------------------------------------------

class PEPAvatar : public QObject, public CachedAvatar
{
	Q_OBJECT

public:
	PEPAvatar(AvatarFactory* factory, const Jid& jid) : jid_(jid), factory_(factory) { };
	
	void setData(const QString& h, const QString& data) {
		if (h == hash()) {
			QByteArray ba = Base64().stringToArray(data).toByteArray();
			if (!ba.isEmpty()) {
				saveToCache(hash(),ba);
				setImage(ba);
				if (pixmap().isNull()) {
					qWarning("PEPAvatar::setData(): Null pixmap. Unsupported format ?");
				}
				emit avatarChanged(jid_);
			}
			else 
				qWarning("PEPAvatar::setData(): Received data is empty. Bad encoding ?");
		}
	}
	
signals:
	void avatarChanged(const Jid&);

protected:
	void requestAvatar() {
		factory_->account()->pepManager()->get(jid_,"http://jabber.org/protocol/avatar#data",hash());
	}

	void avatarUpdated() 
		{ emit avatarChanged(jid_); }

private:
	Jid jid_;
	AvatarFactory* factory_;
};

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// VCardAvatar: Avatars coming from VCards of contacts.
//------------------------------------------------------------------------------

class VCardAvatar : public QObject, public CachedAvatar
{
	Q_OBJECT

public:
	VCardAvatar(AvatarFactory* factory, const Jid& jid);

signals:
	void avatarChanged(const Jid&);

public slots:
	void receivedVCard();

protected:
	void requestAvatar();
	void avatarUpdated() 
		{ emit avatarChanged(jid_); }

private:
	Jid jid_;
	AvatarFactory* factory_;
};


VCardAvatar::VCardAvatar(AvatarFactory* factory, const Jid& jid) : jid_(jid), factory_(factory)
{
}

void VCardAvatar::requestAvatar()
{
	VCardFactory::instance()->getVCard(jid_.bare(),factory_->account()->client()->rootTask(), this, SLOT(receivedVCard()));
}

void VCardAvatar::receivedVCard()
{
	const VCard* vcard = VCardFactory::instance()->vcard(jid_);
	if (vcard) {
		saveToCache(hash(),vcard->photo());
		setImage(vcard->photo());
		emit avatarChanged(jid_);
	}
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// VCardStaticAvatar: VCard static photo avatar (not published through presence)
//------------------------------------------------------------------------------

class VCardStaticAvatar : public QObject, public Avatar
{
	Q_OBJECT

public:
	VCardStaticAvatar(const Jid& j);

public slots:
	void vcardChanged(const Jid&);

signals:
	void avatarChanged(const Jid&);

private:
	Jid jid_;
};


VCardStaticAvatar::VCardStaticAvatar(const Jid& j) : jid_(j.bare()) 
{ 
	const VCard* vcard = VCardFactory::instance()->vcard(jid_);
	if (vcard && !vcard->photo().isEmpty())
		setImage(vcard->photo());
	connect(VCardFactory::instance(),SIGNAL(vcardChanged(const Jid&)),SLOT(vcardChanged(const Jid&)));
}

void VCardStaticAvatar::vcardChanged(const Jid& j)
{
	if (j.compare(jid_,false)) {
		const VCard* vcard = VCardFactory::instance()->vcard(jid_);
		if (vcard && !vcard->photo().isEmpty())
			setImage(vcard->photo());
		else
			resetImage();
		emit avatarChanged(jid_);
	}
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// FileAvatar: Avatars coming from local files.
//------------------------------------------------------------------------------

class FileAvatar : public Avatar
{
public:
	FileAvatar(const Jid& jid);
	void import(const QString& file);
	void removeFromDisk();
	bool exists();
	QPixmap getPixmap();
	const Jid& getJid() const
		{ return jid_; }

protected:
	bool isDirty() const;
	QString getFileName() const;
	void refresh();
	QDateTime lastModified() const
		{ return lastModified_; }

private:
	QDateTime lastModified_;
	Jid jid_;
};


FileAvatar::FileAvatar(const Jid& jid) : jid_(jid)
{
}

void FileAvatar::import(const QString& file)
{
	if (QFileInfo(file).exists()) {
		QFile source_file(file);
		QFile target_file(getFileName());
		if (source_file.open(QIODevice::ReadOnly) && target_file.open(QIODevice::WriteOnly)) {
			QByteArray ba = source_file.readAll();
			QByteArray data = scaleAvatar(ba);
			target_file.write(data);
		}
	}
}

void FileAvatar::removeFromDisk()
{
	QFile f(getFileName());
	f.remove();
}

bool FileAvatar::exists()
{
	return QFileInfo(getFileName()).exists();
}

QPixmap FileAvatar::getPixmap()
{
	refresh();
	return pixmap();
}

void FileAvatar::refresh()
{
	if (isDirty()) {
		if (QFileInfo(getFileName()).exists()) {
			QImage img(getFileName());
			setImage(QImage(getFileName()));
		}
		else
			resetImage();
	}
}


QString FileAvatar::getFileName() const
{
	QString f = getJid().bare();
	f.replace('@',"_at_");
	return QDir(AvatarFactory::getManualDir()).filePath(f);
}


bool FileAvatar::isDirty() const
{
	return (pixmap().isNull()
			|| !QFileInfo(getFileName()).exists()
			|| QFileInfo(getFileName()).lastModified() > lastModified());
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Avatar factory
//------------------------------------------------------------------------------

AvatarFactory::AvatarFactory(PsiAccount* pa) : pa_(pa)
{
	// Register iconset
	iconset_.addToFactory();

	// Connect signals
	connect(VCardFactory::instance(),SIGNAL(vcardChanged(const Jid&)),this,SLOT(updateAvatar(const Jid&)));
	connect(pa_->client(), SIGNAL(resourceAvailable(const Jid &, const Resource &)), SLOT(resourceAvailable(const Jid &, const Resource &)));

	// PEP
	connect(pa_->pepManager(),SIGNAL(itemPublished(const Jid&, const QString&, const PubSubItem&)),SLOT(itemPublished(const Jid&, const QString&, const PubSubItem&)));
	connect(pa_->pepManager(),SIGNAL(publish_success(const QString&, const PubSubItem&)),SLOT(publish_success(const QString&,const PubSubItem&)));
}

PsiAccount* AvatarFactory::account() const
{
	return pa_;
}

QPixmap AvatarFactory::getAvatar(const Jid& jid)
{
	// Compute the avatar of the user
	Avatar* av = retrieveAvatar(jid);

	// If the avatar changed since the previous request, notify everybody of this
	if (av != active_avatars_[jid.full()]) {
		active_avatars_[jid.full()] = av;
		active_avatars_[jid.bare()] = av;
		emit avatarChanged(jid);
	}

	QPixmap pm = (av ? av->getPixmap() : QPixmap());
	
	// Update iconset
	Icon icon;
	icon.setImpix(pm);
	iconset_.setIcon(QString("avatars/%1").arg(jid.bare()),icon);

	return pm;
}

Avatar* AvatarFactory::retrieveAvatar(const Jid& jid)
{
	//printf("Retrieving avatar of %s\n", jid.full().latin1());

	// Try finding a file avatar.
	//printf("File avatar\n");
	if (!file_avatars_.contains(jid.bare())) {
		//printf("File avatar not yet loaded\n");
		file_avatars_[jid.bare()] = new FileAvatar(jid);
	}
	//printf("Trying file avatar\n");
	if (!file_avatars_[jid.bare()]->isEmpty())
		return file_avatars_[jid.bare()];
	
	//printf("PEP avatar\n");
	if (pep_avatars_.contains(jid.bare()) && !pep_avatars_[jid.bare()]->isEmpty()) {
		return pep_avatars_[jid.bare()];
	}

	// Try finding a vcard avatar
	//printf("VCard avatar\n");
	if (vcard_avatars_.contains(jid.bare()) && !vcard_avatars_[jid.bare()]->isEmpty()) {
		return vcard_avatars_[jid.bare()];
	}
	
	// Try finding a static vcard avatar
	//printf("Static VCard avatar\n");
	if (!vcard_static_avatars_.contains(jid.bare())) {
		//printf("Static vcard avatar not yet loaded\n");
		vcard_static_avatars_[jid.bare()] = new VCardStaticAvatar(jid);
		connect(vcard_static_avatars_[jid.bare()],SIGNAL(avatarChanged(const Jid&)),this,SLOT(updateAvatar(const Jid&)));
	}
	if (!vcard_static_avatars_[jid.bare()]->isEmpty()) {
		return vcard_static_avatars_[jid.bare()];
	}

	return 0;
}

void AvatarFactory::setSelfAvatar(const QString& fileName)
{
	if (!fileName.isEmpty()) {
		QFile avatar_file(fileName);
		if (!avatar_file.open(QIODevice::ReadOnly))
			return;
		
		QByteArray avatar_data = scaleAvatar(avatar_file.readAll());
		QImage avatar_image(avatar_data);
		if(!avatar_image.isNull()) {
			// Publish data
			QDomDocument* doc = account()->client()->doc();
			QString hash = SHA1().hashToString(avatar_data);
			QDomElement el = doc->createElement("data");
			el.setAttribute("xmlns","http://jabber.org/protocol/avatar#data");
			el.appendChild(doc->createTextNode(Base64().arrayToString(avatar_data)));
			selfAvatarData_ = avatar_data;
			selfAvatarHash_ = hash;
			account()->pepManager()->publish("http://jabber.org/protocol/avatar#data",PubSubItem(hash,el));
		}
	}
	else {
		QDomDocument* doc = account()->client()->doc();
		QDomElement meta_el =  doc->createElement("metadata");
		meta_el.setAttribute("xmlns","http://jabber.org/protocol/avatar#metadata");
		meta_el.appendChild(doc->createElement("stop"));
		account()->pepManager()->publish("http://jabber.org/protocol/avatar#metadata",PubSubItem("current",meta_el));
	}
}

void AvatarFactory::updateAvatar(const Jid& j)
{
	getAvatar(j);
	// FIXME: This signal might be emitted twice (first time from getAvatar()).
	emit avatarChanged(j);
}

void AvatarFactory::importManualAvatar(const Jid& j, const QString& fileName)
{
	FileAvatar(j).import(fileName);
	emit avatarChanged(j);
}

void AvatarFactory::removeManualAvatar(const Jid& j)
{
	FileAvatar(j).removeFromDisk();
	// TODO: Remove from caches. Maybe create a clearManualAvatar() which
	// removes the file but doesn't remove the avatar from caches (since it'll
	// be created again whenever the FileAvatar is requested)
	emit avatarChanged(j);
}

bool AvatarFactory::hasManualAvatar(const Jid& j)
{
	return FileAvatar(j).exists();
}

void AvatarFactory::resourceAvailable(const Jid& jid, const Resource& r)
{
	if (r.status().hasPhotoHash()) {
		QString hash = r.status().photoHash();
		if (!vcard_avatars_.contains(jid.bare())) {
			vcard_avatars_[jid.bare()] = new VCardAvatar(this, jid);
			connect(vcard_avatars_[jid.bare()],SIGNAL(avatarChanged(const Jid&)),this,SLOT(updateAvatar(const Jid&)));
		}
		vcard_avatars_[jid.bare()]->updateHash(hash);
	}
}

QString AvatarFactory::getManualDir()
{
	QDir avatars(pathToProfile(activeProfile) + "/pictures");
	if (!avatars.exists()) {
		QDir profile(pathToProfile(activeProfile));
		profile.mkdir("pictures");
	}
	return avatars.path();
}

QString AvatarFactory::getCacheDir()
{
	QDir avatars(getHomeDir() + "/avatars");
	if (!avatars.exists()) {
		QDir home(getHomeDir());
		home.mkdir("avatars");
	}
	return avatars.path();
}

void AvatarFactory::itemPublished(const Jid& jid, const QString& n, const PubSubItem& item)
{
	if (n == "http://jabber.org/protocol/avatar#data") {
		if (item.payload().tagName() == "data") {
			pep_avatars_[jid.bare()]->setData(item.id(),item.payload().text());
		}
		else {
			qWarning("avatars.cpp: Unexpected item payload");
		}
	}
	else if (n == "http://jabber.org/protocol/avatar#metadata") {
		if (!pep_avatars_.contains(jid.bare())) {
			pep_avatars_[jid.bare()] = new PEPAvatar(this, jid.bare());
			connect(pep_avatars_[jid.bare()],SIGNAL(avatarChanged(const Jid&)),this, SLOT(updateAvatar(const Jid&)));
		}
		QDomElement e;
		bool found;
		e = findSubTag(item.payload(), "stop", &found);
		if (found) {
			pep_avatars_[jid.bare()]->updateHash("");
		}
		else {
			pep_avatars_[jid.bare()]->updateHash(item.id());
		}
	}	
}

void AvatarFactory::publish_success(const QString& n, const PubSubItem& item)
{
	if (n == "http://jabber.org/protocol/avatar#data" && item.id() == selfAvatarHash_) {
		// Publish metadata
		QDomDocument* doc = account()->client()->doc();
		QImage avatar_image(selfAvatarData_);
		QDomElement meta_el = doc->createElement("metadata");
		meta_el.setAttribute("xmlns","http://jabber.org/protocol/avatar#metadata");
		QDomElement info_el = doc->createElement("info");
		info_el.setAttribute("id",selfAvatarHash_);
		info_el.setAttribute("bytes",avatar_image.numBytes());
		info_el.setAttribute("height",avatar_image.height());
		info_el.setAttribute("width",avatar_image.width());
		info_el.setAttribute("type",image2type(selfAvatarData_));
		meta_el.appendChild(info_el);
		account()->pepManager()->publish("http://jabber.org/protocol/avatar#metadata",PubSubItem(selfAvatarHash_,meta_el));
	}
}

//------------------------------------------------------------------------------

#include "avatars.moc"