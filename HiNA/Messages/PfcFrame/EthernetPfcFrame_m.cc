//
// Generated file, do not edit! Created by opp_msgtool 6.0 from inet/HiNA/Messages/PfcFrame/EthernetPfcFrame.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "EthernetPfcFrame_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace inet {

Register_Enum(inet::EthernetPfcControlOpCode, (inet::EthernetPfcControlOpCode::ETHERNET_PFC_PAUSE, inet::EthernetPfcControlOpCode::ETHERNET_PFC_RESUME));

Register_Class(EthernetPfcFrame)

EthernetPfcFrame::EthernetPfcFrame() : ::inet::EthernetControlFrameBase()
{
    this->setChunkLength(B(20));

}

EthernetPfcFrame::EthernetPfcFrame(const EthernetPfcFrame& other) : ::inet::EthernetControlFrameBase(other)
{
    copy(other);
}

EthernetPfcFrame::~EthernetPfcFrame()
{
}

EthernetPfcFrame& EthernetPfcFrame::operator=(const EthernetPfcFrame& other)
{
    if (this == &other) return *this;
    ::inet::EthernetControlFrameBase::operator=(other);
    copy(other);
    return *this;
}

void EthernetPfcFrame::copy(const EthernetPfcFrame& other)
{
    this->priority = other.priority;
    this->pauseTime = other.pauseTime;
}

void EthernetPfcFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::EthernetControlFrameBase::parsimPack(b);
    doParsimPacking(b,this->priority);
    doParsimPacking(b,this->pauseTime);
}

void EthernetPfcFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::EthernetControlFrameBase::parsimUnpack(b);
    doParsimUnpacking(b,this->priority);
    doParsimUnpacking(b,this->pauseTime);
}

int16_t EthernetPfcFrame::getPriority() const
{
    return this->priority;
}

void EthernetPfcFrame::setPriority(int16_t priority)
{
    handleChange();
    this->priority = priority;
}

int16_t EthernetPfcFrame::getPauseTime() const
{
    return this->pauseTime;
}

void EthernetPfcFrame::setPauseTime(int16_t pauseTime)
{
    handleChange();
    this->pauseTime = pauseTime;
}

class EthernetPfcFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_priority,
        FIELD_pauseTime,
    };
  public:
    EthernetPfcFrameDescriptor();
    virtual ~EthernetPfcFrameDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(EthernetPfcFrameDescriptor)

EthernetPfcFrameDescriptor::EthernetPfcFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetPfcFrame)), "inet::EthernetControlFrameBase")
{
    propertyNames = nullptr;
}

EthernetPfcFrameDescriptor::~EthernetPfcFrameDescriptor()
{
    delete[] propertyNames;
}

bool EthernetPfcFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetPfcFrame *>(obj)!=nullptr;
}

const char **EthernetPfcFrameDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *EthernetPfcFrameDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int EthernetPfcFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 2+base->getFieldCount() : 2;
}

unsigned int EthernetPfcFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_priority
        FD_ISEDITABLE,    // FIELD_pauseTime
    };
    return (field >= 0 && field < 2) ? fieldTypeFlags[field] : 0;
}

const char *EthernetPfcFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "priority",
        "pauseTime",
    };
    return (field >= 0 && field < 2) ? fieldNames[field] : nullptr;
}

int EthernetPfcFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "priority") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "pauseTime") == 0) return baseIndex + 1;
    return base ? base->findField(fieldName) : -1;
}

const char *EthernetPfcFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int16_t",    // FIELD_priority
        "int16_t",    // FIELD_pauseTime
    };
    return (field >= 0 && field < 2) ? fieldTypeStrings[field] : nullptr;
}

const char **EthernetPfcFrameDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *EthernetPfcFrameDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int EthernetPfcFrameDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void EthernetPfcFrameDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'EthernetPfcFrame'", field);
    }
}

const char *EthernetPfcFrameDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EthernetPfcFrameDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        case FIELD_priority: return long2string(pp->getPriority());
        case FIELD_pauseTime: return long2string(pp->getPauseTime());
        default: return "";
    }
}

void EthernetPfcFrameDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        case FIELD_priority: pp->setPriority(string2long(value)); break;
        case FIELD_pauseTime: pp->setPauseTime(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EthernetPfcFrame'", field);
    }
}

omnetpp::cValue EthernetPfcFrameDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        case FIELD_priority: return pp->getPriority();
        case FIELD_pauseTime: return pp->getPauseTime();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'EthernetPfcFrame' as cValue -- field index out of range?", field);
    }
}

void EthernetPfcFrameDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        case FIELD_priority: pp->setPriority(omnetpp::checked_int_cast<int16_t>(value.intValue())); break;
        case FIELD_pauseTime: pp->setPauseTime(omnetpp::checked_int_cast<int16_t>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EthernetPfcFrame'", field);
    }
}

const char *EthernetPfcFrameDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr EthernetPfcFrameDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void EthernetPfcFrameDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    EthernetPfcFrame *pp = omnetpp::fromAnyPtr<EthernetPfcFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'EthernetPfcFrame'", field);
    }
}

}  // namespace inet

namespace omnetpp {

}  // namespace omnetpp

