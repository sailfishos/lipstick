#ifndef MDCONFITEM_STUB
#define MDCONFITEM_STUB

#include "mdconfitem.h"
#include <stubbase.h>


// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class MDConfItemStub : public StubBase
{
public:
    virtual void MDConfItemConstructor(const QString &key, QObject *parent);
    virtual void MDConfItemDestructor();
    virtual QString key() const;
    virtual QVariant value() const;
    virtual QVariant value(const QVariant &def) const;
    virtual void set(const QVariant &val);
    virtual void unset();
};

// 2. IMPLEMENT STUB
void MDConfItemStub::MDConfItemConstructor(const QString &key, QObject *parent)
{
    Q_UNUSED(key);
    Q_UNUSED(parent);

}
void MDConfItemStub::MDConfItemDestructor()
{

}
QString MDConfItemStub::key() const
{
    stubMethodEntered("key");
    return stubReturnValue<QString>("key");
}

QVariant MDConfItemStub::value() const
{
    stubMethodEntered("value");
    return stubReturnValue<QVariant>("value");
}

QVariant MDConfItemStub::value(const QVariant &def) const
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QVariant & >(def));
    stubMethodEntered("value", params);
    return stubReturnValue<QVariant>("value");
}

void MDConfItemStub::set(const QVariant &val)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QVariant & >(val));
    stubMethodEntered("set", params);
}

void MDConfItemStub::unset()
{
    stubMethodEntered("unset");
}



// 3. CREATE A STUB INSTANCE
MDConfItemStub gDefaultMDConfItemStub;
MDConfItemStub *gMDConfItemStub = &gDefaultMDConfItemStub;


// 4. CREATE A PROXY WHICH CALLS THE STUB
MDConfItem::MDConfItem(const QString &key, QObject *parent)
{
    gMDConfItemStub->MDConfItemConstructor(key, parent);
}

MDConfItem::~MDConfItem()
{
    gMDConfItemStub->MDConfItemDestructor();
}

QString MDConfItem::key() const
{
    return gMDConfItemStub->key();
}

QVariant MDConfItem::value() const
{
    return gMDConfItemStub->value();
}

QVariant MDConfItem::value(const QVariant &def) const
{
    return gMDConfItemStub->value(def);
}

void MDConfItem::set(const QVariant &val)
{
    gMDConfItemStub->set(val);
}

void MDConfItem::unset()
{
    gMDConfItemStub->unset();
}


#endif
