//
// Generated file, do not edit! Created by opp_msgtool 6.0 from inet/HiNA/Messages/HiTag/HiTag.msg.
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
#include "inet\HiNA\Messages\HiTag\HiTag_m.h"

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

Register_Class(HiTag)

HiTag::HiTag() : ::inet::TagBase()
{
}

HiTag::HiTag(const HiTag& other) : ::inet::TagBase(other)
{
    copy(other);
}

HiTag::~HiTag()
{
}

HiTag& HiTag::operator=(const HiTag& other)
{
    if (this == &other) return *this;
    ::inet::TagBase::operator=(other);
    copy(other);
    return *this;
}

void HiTag::copy(const HiTag& other)
{
    this->Priority = other.Priority;
    this->PacketId = other.PacketId;
    this->FirstRttPcks = other.FirstRttPcks;
    this->FlowId = other.FlowId;
    this->FlowSize = other.FlowSize;
    this->PacketSize = other.PacketSize;
    this->reverse = other.reverse;
    this->creationtime = other.creationtime;
    this->isLastPck_ = other.isLastPck_;
    this->op = other.op;
    this->interfaceId = other.interfaceId;
}

void HiTag::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::TagBase::parsimPack(b);
    doParsimPacking(b,this->Priority);
    doParsimPacking(b,this->PacketId);
    doParsimPacking(b,this->FirstRttPcks);
    doParsimPacking(b,this->FlowId);
    doParsimPacking(b,this->FlowSize);
    doParsimPacking(b,this->PacketSize);
    doParsimPacking(b,this->reverse);
    doParsimPacking(b,this->creationtime);
    doParsimPacking(b,this->isLastPck_);
    doParsimPacking(b,this->op);
    doParsimPacking(b,this->interfaceId);
}

void HiTag::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::TagBase::parsimUnpack(b);
    doParsimUnpacking(b,this->Priority);
    doParsimUnpacking(b,this->PacketId);
    doParsimUnpacking(b,this->FirstRttPcks);
    doParsimUnpacking(b,this->FlowId);
    doParsimUnpacking(b,this->FlowSize);
    doParsimUnpacking(b,this->PacketSize);
    doParsimUnpacking(b,this->reverse);
    doParsimUnpacking(b,this->creationtime);
    doParsimUnpacking(b,this->isLastPck_);
    doParsimUnpacking(b,this->op);
    doParsimUnpacking(b,this->interfaceId);
}

uint32_t HiTag::getPriority() const
{
    return this->Priority;
}

void HiTag::setPriority(uint32_t Priority)
{
    this->Priority = Priority;
}

uint64_t HiTag::getPacketId() const
{
    return this->PacketId;
}

void HiTag::setPacketId(uint64_t PacketId)
{
    this->PacketId = PacketId;
}

uint64_t HiTag::getFirstRttPcks() const
{
    return this->FirstRttPcks;
}

void HiTag::setFirstRttPcks(uint64_t FirstRttPcks)
{
    this->FirstRttPcks = FirstRttPcks;
}

uint32_t HiTag::getFlowId() const
{
    return this->FlowId;
}

void HiTag::setFlowId(uint32_t FlowId)
{
    this->FlowId = FlowId;
}

uint64_t HiTag::getFlowSize() const
{
    return this->FlowSize;
}

void HiTag::setFlowSize(uint64_t FlowSize)
{
    this->FlowSize = FlowSize;
}

uint64_t HiTag::getPacketSize() const
{
    return this->PacketSize;
}

void HiTag::setPacketSize(uint64_t PacketSize)
{
    this->PacketSize = PacketSize;
}

bool HiTag::getReverse() const
{
    return this->reverse;
}

void HiTag::setReverse(bool reverse)
{
    this->reverse = reverse;
}

::omnetpp::simtime_t HiTag::getCreationtime() const
{
    return this->creationtime;
}

void HiTag::setCreationtime(::omnetpp::simtime_t creationtime)
{
    this->creationtime = creationtime;
}

bool HiTag::isLastPck() const
{
    return this->isLastPck_;
}

void HiTag::setIsLastPck(bool isLastPck)
{
    this->isLastPck_ = isLastPck;
}

int16_t HiTag::getOp() const
{
    return this->op;
}

void HiTag::setOp(int16_t op)
{
    this->op = op;
}

int16_t HiTag::getInterfaceId() const
{
    return this->interfaceId;
}

void HiTag::setInterfaceId(int16_t interfaceId)
{
    this->interfaceId = interfaceId;
}

class HiTagDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_Priority,
        FIELD_PacketId,
        FIELD_FirstRttPcks,
        FIELD_FlowId,
        FIELD_FlowSize,
        FIELD_PacketSize,
        FIELD_reverse,
        FIELD_creationtime,
        FIELD_isLastPck,
        FIELD_op,
        FIELD_interfaceId,
    };
  public:
    HiTagDescriptor();
    virtual ~HiTagDescriptor();

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

Register_ClassDescriptor(HiTagDescriptor)

HiTagDescriptor::HiTagDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::HiTag)), "inet::TagBase")
{
    propertyNames = nullptr;
}

HiTagDescriptor::~HiTagDescriptor()
{
    delete[] propertyNames;
}

bool HiTagDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<HiTag *>(obj)!=nullptr;
}

const char **HiTagDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *HiTagDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int HiTagDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 11+base->getFieldCount() : 11;
}

unsigned int HiTagDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_Priority
        FD_ISEDITABLE,    // FIELD_PacketId
        FD_ISEDITABLE,    // FIELD_FirstRttPcks
        FD_ISEDITABLE,    // FIELD_FlowId
        FD_ISEDITABLE,    // FIELD_FlowSize
        FD_ISEDITABLE,    // FIELD_PacketSize
        FD_ISEDITABLE,    // FIELD_reverse
        FD_ISEDITABLE,    // FIELD_creationtime
        FD_ISEDITABLE,    // FIELD_isLastPck
        FD_ISEDITABLE,    // FIELD_op
        FD_ISEDITABLE,    // FIELD_interfaceId
    };
    return (field >= 0 && field < 11) ? fieldTypeFlags[field] : 0;
}

const char *HiTagDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "Priority",
        "PacketId",
        "FirstRttPcks",
        "FlowId",
        "FlowSize",
        "PacketSize",
        "reverse",
        "creationtime",
        "isLastPck",
        "op",
        "interfaceId",
    };
    return (field >= 0 && field < 11) ? fieldNames[field] : nullptr;
}

int HiTagDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "Priority") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "PacketId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "FirstRttPcks") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "FlowId") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "FlowSize") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "PacketSize") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "reverse") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "creationtime") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "isLastPck") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "op") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "interfaceId") == 0) return baseIndex + 10;
    return base ? base->findField(fieldName) : -1;
}

const char *HiTagDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "uint32_t",    // FIELD_Priority
        "uint64_t",    // FIELD_PacketId
        "uint64_t",    // FIELD_FirstRttPcks
        "uint32_t",    // FIELD_FlowId
        "uint64_t",    // FIELD_FlowSize
        "uint64_t",    // FIELD_PacketSize
        "bool",    // FIELD_reverse
        "omnetpp::simtime_t",    // FIELD_creationtime
        "bool",    // FIELD_isLastPck
        "int16_t",    // FIELD_op
        "int16_t",    // FIELD_interfaceId
    };
    return (field >= 0 && field < 11) ? fieldTypeStrings[field] : nullptr;
}

const char **HiTagDescriptor::getFieldPropertyNames(int field) const
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

const char *HiTagDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int HiTagDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void HiTagDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'HiTag'", field);
    }
}

const char *HiTagDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string HiTagDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        case FIELD_Priority: return ulong2string(pp->getPriority());
        case FIELD_PacketId: return uint642string(pp->getPacketId());
        case FIELD_FirstRttPcks: return uint642string(pp->getFirstRttPcks());
        case FIELD_FlowId: return ulong2string(pp->getFlowId());
        case FIELD_FlowSize: return uint642string(pp->getFlowSize());
        case FIELD_PacketSize: return uint642string(pp->getPacketSize());
        case FIELD_reverse: return bool2string(pp->getReverse());
        case FIELD_creationtime: return simtime2string(pp->getCreationtime());
        case FIELD_isLastPck: return bool2string(pp->isLastPck());
        case FIELD_op: return long2string(pp->getOp());
        case FIELD_interfaceId: return long2string(pp->getInterfaceId());
        default: return "";
    }
}

void HiTagDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        case FIELD_Priority: pp->setPriority(string2ulong(value)); break;
        case FIELD_PacketId: pp->setPacketId(string2uint64(value)); break;
        case FIELD_FirstRttPcks: pp->setFirstRttPcks(string2uint64(value)); break;
        case FIELD_FlowId: pp->setFlowId(string2ulong(value)); break;
        case FIELD_FlowSize: pp->setFlowSize(string2uint64(value)); break;
        case FIELD_PacketSize: pp->setPacketSize(string2uint64(value)); break;
        case FIELD_reverse: pp->setReverse(string2bool(value)); break;
        case FIELD_creationtime: pp->setCreationtime(string2simtime(value)); break;
        case FIELD_isLastPck: pp->setIsLastPck(string2bool(value)); break;
        case FIELD_op: pp->setOp(string2long(value)); break;
        case FIELD_interfaceId: pp->setInterfaceId(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'HiTag'", field);
    }
}

omnetpp::cValue HiTagDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        case FIELD_Priority: return (omnetpp::intval_t)(pp->getPriority());
        case FIELD_PacketId: return (omnetpp::intval_t)(pp->getPacketId());
        case FIELD_FirstRttPcks: return (omnetpp::intval_t)(pp->getFirstRttPcks());
        case FIELD_FlowId: return (omnetpp::intval_t)(pp->getFlowId());
        case FIELD_FlowSize: return (omnetpp::intval_t)(pp->getFlowSize());
        case FIELD_PacketSize: return (omnetpp::intval_t)(pp->getPacketSize());
        case FIELD_reverse: return pp->getReverse();
        case FIELD_creationtime: return pp->getCreationtime().dbl();
        case FIELD_isLastPck: return pp->isLastPck();
        case FIELD_op: return pp->getOp();
        case FIELD_interfaceId: return pp->getInterfaceId();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'HiTag' as cValue -- field index out of range?", field);
    }
}

void HiTagDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        case FIELD_Priority: pp->setPriority(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_PacketId: pp->setPacketId(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_FirstRttPcks: pp->setFirstRttPcks(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_FlowId: pp->setFlowId(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_FlowSize: pp->setFlowSize(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_PacketSize: pp->setPacketSize(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_reverse: pp->setReverse(value.boolValue()); break;
        case FIELD_creationtime: pp->setCreationtime(value.doubleValue()); break;
        case FIELD_isLastPck: pp->setIsLastPck(value.boolValue()); break;
        case FIELD_op: pp->setOp(omnetpp::checked_int_cast<int16_t>(value.intValue())); break;
        case FIELD_interfaceId: pp->setInterfaceId(omnetpp::checked_int_cast<int16_t>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'HiTag'", field);
    }
}

const char *HiTagDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr HiTagDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void HiTagDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    HiTag *pp = omnetpp::fromAnyPtr<HiTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'HiTag'", field);
    }
}

}  // namespace inet

namespace omnetpp {

}  // namespace omnetpp

