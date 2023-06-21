//
// Generated file, do not edit! Created by opp_msgtool 6.0 from inet/HiNA/Messages/HPCC/INTHeader.msg.
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
#include "INTHeader_m.h"

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

hopInf::hopInf()
{
}

void __doPacking(omnetpp::cCommBuffer *b, const hopInf& a)
{
    doParsimPacking(b,a.queueLength);
    doParsimPacking(b,a.txRate);
    doParsimPacking(b,a.txBytes);
    doParsimPacking(b,a.TS);
}

void __doUnpacking(omnetpp::cCommBuffer *b, hopInf& a)
{
    doParsimUnpacking(b,a.queueLength);
    doParsimUnpacking(b,a.txRate);
    doParsimUnpacking(b,a.txBytes);
    doParsimUnpacking(b,a.TS);
}

class hopInfDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_queueLength,
        FIELD_txRate,
        FIELD_txBytes,
        FIELD_TS,
    };
  public:
    hopInfDescriptor();
    virtual ~hopInfDescriptor();

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

Register_ClassDescriptor(hopInfDescriptor)

hopInfDescriptor::hopInfDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::hopInf)), "")
{
    propertyNames = nullptr;
}

hopInfDescriptor::~hopInfDescriptor()
{
    delete[] propertyNames;
}

bool hopInfDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<hopInf *>(obj)!=nullptr;
}

const char **hopInfDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *hopInfDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int hopInfDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 4+base->getFieldCount() : 4;
}

unsigned int hopInfDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_queueLength
        FD_ISEDITABLE,    // FIELD_txRate
        FD_ISEDITABLE,    // FIELD_txBytes
        FD_ISEDITABLE,    // FIELD_TS
    };
    return (field >= 0 && field < 4) ? fieldTypeFlags[field] : 0;
}

const char *hopInfDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "queueLength",
        "txRate",
        "txBytes",
        "TS",
    };
    return (field >= 0 && field < 4) ? fieldNames[field] : nullptr;
}

int hopInfDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "queueLength") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "txRate") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "txBytes") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "TS") == 0) return baseIndex + 3;
    return base ? base->findField(fieldName) : -1;
}

const char *hopInfDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::b",    // FIELD_queueLength
        "double",    // FIELD_txRate
        "uint64",    // FIELD_txBytes
        "omnetpp::simtime_t",    // FIELD_TS
    };
    return (field >= 0 && field < 4) ? fieldTypeStrings[field] : nullptr;
}

const char **hopInfDescriptor::getFieldPropertyNames(int field) const
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

const char *hopInfDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int hopInfDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void hopInfDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'hopInf'", field);
    }
}

const char *hopInfDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string hopInfDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        case FIELD_queueLength: return unit2string(pp->queueLength);
        case FIELD_txRate: return double2string(pp->txRate);
        case FIELD_txBytes: return uint642string(pp->txBytes);
        case FIELD_TS: return simtime2string(pp->TS);
        default: return "";
    }
}

void hopInfDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        case FIELD_queueLength: pp->queueLength = b(string2long(value)); break;
        case FIELD_txRate: pp->txRate = string2double(value); break;
        case FIELD_txBytes: pp->txBytes = string2uint64(value); break;
        case FIELD_TS: pp->TS = string2simtime(value); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'hopInf'", field);
    }
}

omnetpp::cValue hopInfDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        case FIELD_queueLength: return cValue(pp->queueLength.get(),"b");
        case FIELD_txRate: return pp->txRate;
        case FIELD_txBytes: return omnetpp::checked_int_cast<omnetpp::intval_t>(pp->txBytes);
        case FIELD_TS: return pp->TS.dbl();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'hopInf' as cValue -- field index out of range?", field);
    }
}

void hopInfDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        case FIELD_queueLength: pp->queueLength = b(value.intValueInUnit("b")); break;
        case FIELD_txRate: pp->txRate = value.doubleValue(); break;
        case FIELD_txBytes: pp->txBytes = omnetpp::checked_int_cast<uint64_t>(value.intValue()); break;
        case FIELD_TS: pp->TS = value.doubleValue(); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'hopInf'", field);
    }
}

const char *hopInfDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr hopInfDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void hopInfDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    hopInf *pp = omnetpp::fromAnyPtr<hopInf>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'hopInf'", field);
    }
}

Register_Class(INTHeader)

INTHeader::INTHeader() : ::inet::FieldsChunk()
{
    this->setChunkLength(B (42));

}

INTHeader::INTHeader(const INTHeader& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

INTHeader::~INTHeader()
{
}

INTHeader& INTHeader::operator=(const INTHeader& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void INTHeader::copy(const INTHeader& other)
{
    this->nHop = other.nHop;
    this->pathID = other.pathID;
    for (size_t i = 0; i < 5; i++) {
        this->hopInfs[i] = other.hopInfs[i];
    }
}

void INTHeader::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->nHop);
    doParsimPacking(b,this->pathID);
    doParsimArrayPacking(b,this->hopInfs,5);
}

void INTHeader::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->nHop);
    doParsimUnpacking(b,this->pathID);
    doParsimArrayUnpacking(b,this->hopInfs,5);
}

int INTHeader::getNHop() const
{
    return this->nHop;
}

void INTHeader::setNHop(int nHop)
{
    handleChange();
    this->nHop = nHop;
}

int INTHeader::getPathID() const
{
    return this->pathID;
}

void INTHeader::setPathID(int pathID)
{
    handleChange();
    this->pathID = pathID;
}

size_t INTHeader::getHopInfsArraySize() const
{
    return 5;
}

const hopInf& INTHeader::getHopInfs(size_t k) const
{
    if (k >= 5) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)5, (unsigned long)k);
    return this->hopInfs[k];
}

void INTHeader::setHopInfs(size_t k, const hopInf& hopInfs)
{
    if (k >= 5) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)5, (unsigned long)k);
    handleChange();
    this->hopInfs[k] = hopInfs;
}

class INTHeaderDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_nHop,
        FIELD_pathID,
        FIELD_hopInfs,
    };
  public:
    INTHeaderDescriptor();
    virtual ~INTHeaderDescriptor();

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

Register_ClassDescriptor(INTHeaderDescriptor)

INTHeaderDescriptor::INTHeaderDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::INTHeader)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

INTHeaderDescriptor::~INTHeaderDescriptor()
{
    delete[] propertyNames;
}

bool INTHeaderDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<INTHeader *>(obj)!=nullptr;
}

const char **INTHeaderDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *INTHeaderDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int INTHeaderDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 3+base->getFieldCount() : 3;
}

unsigned int INTHeaderDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_nHop
        FD_ISEDITABLE,    // FIELD_pathID
        FD_ISARRAY | FD_ISCOMPOUND,    // FIELD_hopInfs
    };
    return (field >= 0 && field < 3) ? fieldTypeFlags[field] : 0;
}

const char *INTHeaderDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "nHop",
        "pathID",
        "hopInfs",
    };
    return (field >= 0 && field < 3) ? fieldNames[field] : nullptr;
}

int INTHeaderDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "nHop") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "pathID") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "hopInfs") == 0) return baseIndex + 2;
    return base ? base->findField(fieldName) : -1;
}

const char *INTHeaderDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_nHop
        "int",    // FIELD_pathID
        "inet::hopInf",    // FIELD_hopInfs
    };
    return (field >= 0 && field < 3) ? fieldTypeStrings[field] : nullptr;
}

const char **INTHeaderDescriptor::getFieldPropertyNames(int field) const
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

const char *INTHeaderDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int INTHeaderDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_hopInfs: return 5;
        default: return 0;
    }
}

void INTHeaderDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'INTHeader'", field);
    }
}

const char *INTHeaderDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string INTHeaderDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_nHop: return long2string(pp->getNHop());
        case FIELD_pathID: return long2string(pp->getPathID());
        case FIELD_hopInfs: return "";
        default: return "";
    }
}

void INTHeaderDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_nHop: pp->setNHop(string2long(value)); break;
        case FIELD_pathID: pp->setPathID(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'INTHeader'", field);
    }
}

omnetpp::cValue INTHeaderDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_nHop: return pp->getNHop();
        case FIELD_pathID: return pp->getPathID();
        case FIELD_hopInfs: return omnetpp::toAnyPtr(&pp->getHopInfs(i)); break;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'INTHeader' as cValue -- field index out of range?", field);
    }
}

void INTHeaderDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_nHop: pp->setNHop(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_pathID: pp->setPathID(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'INTHeader'", field);
    }
}

const char *INTHeaderDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_hopInfs: return omnetpp::opp_typename(typeid(hopInf));
        default: return nullptr;
    };
}

omnetpp::any_ptr INTHeaderDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        case FIELD_hopInfs: return omnetpp::toAnyPtr(&pp->getHopInfs(i)); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void INTHeaderDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    INTHeader *pp = omnetpp::fromAnyPtr<INTHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'INTHeader'", field);
    }
}

}  // namespace inet

namespace omnetpp {

}  // namespace omnetpp

