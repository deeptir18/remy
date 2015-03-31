// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "problem.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace ProblemBuffers {

namespace {

const ::google::protobuf::Descriptor* Problem_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Problem_reflection_ = NULL;
const ::google::protobuf::Descriptor* ProblemSettings_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  ProblemSettings_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_problem_2eproto() {
  protobuf_AddDesc_problem_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "problem.proto");
  GOOGLE_CHECK(file != NULL);
  Problem_descriptor_ = file->message_type(0);
  static const int Problem_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Problem, settings_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Problem, configs_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Problem, whiskers_),
  };
  Problem_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Problem_descriptor_,
      Problem::default_instance_,
      Problem_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Problem, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Problem, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Problem));
  ProblemSettings_descriptor_ = file->message_type(1);
  static const int ProblemSettings_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ProblemSettings, prng_seed_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ProblemSettings, tick_count_),
  };
  ProblemSettings_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      ProblemSettings_descriptor_,
      ProblemSettings::default_instance_,
      ProblemSettings_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ProblemSettings, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ProblemSettings, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(ProblemSettings));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_problem_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Problem_descriptor_, &Problem::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    ProblemSettings_descriptor_, &ProblemSettings::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_problem_2eproto() {
  delete Problem::default_instance_;
  delete Problem_reflection_;
  delete ProblemSettings::default_instance_;
  delete ProblemSettings_reflection_;
}

void protobuf_AddDesc_problem_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::RemyBuffers::protobuf_AddDesc_dna_2eproto();
  ::AnswerBuffers::protobuf_AddDesc_answer_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\rproblem.proto\022\016ProblemBuffers\032\tdna.pro"
    "to\032\014answer.proto\"\221\001\n\007Problem\0221\n\010settings"
    "\030\001 \001(\0132\037.ProblemBuffers.ProblemSettings\022"
    "\'\n\007configs\030\002 \003(\0132\026.RemyBuffers.NetConfig"
    "\022*\n\010whiskers\030\003 \001(\0132\030.RemyBuffers.Whisker"
    "Tree\"8\n\017ProblemSettings\022\021\n\tprng_seed\030\013 \001"
    "(\r\022\022\n\ntick_count\030\014 \001(\r", 262);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "problem.proto", &protobuf_RegisterTypes);
  Problem::default_instance_ = new Problem();
  ProblemSettings::default_instance_ = new ProblemSettings();
  Problem::default_instance_->InitAsDefaultInstance();
  ProblemSettings::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_problem_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_problem_2eproto {
  StaticDescriptorInitializer_problem_2eproto() {
    protobuf_AddDesc_problem_2eproto();
  }
} static_descriptor_initializer_problem_2eproto_;


// ===================================================================

#ifndef _MSC_VER
const int Problem::kSettingsFieldNumber;
const int Problem::kConfigsFieldNumber;
const int Problem::kWhiskersFieldNumber;
#endif  // !_MSC_VER

Problem::Problem()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void Problem::InitAsDefaultInstance() {
  settings_ = const_cast< ::ProblemBuffers::ProblemSettings*>(&::ProblemBuffers::ProblemSettings::default_instance());
  whiskers_ = const_cast< ::RemyBuffers::WhiskerTree*>(&::RemyBuffers::WhiskerTree::default_instance());
}

Problem::Problem(const Problem& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void Problem::SharedCtor() {
  _cached_size_ = 0;
  settings_ = NULL;
  whiskers_ = NULL;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Problem::~Problem() {
  SharedDtor();
}

void Problem::SharedDtor() {
  if (this != default_instance_) {
    delete settings_;
    delete whiskers_;
  }
}

void Problem::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Problem::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Problem_descriptor_;
}

const Problem& Problem::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_problem_2eproto();  return *default_instance_;
}

Problem* Problem::default_instance_ = NULL;

Problem* Problem::New() const {
  return new Problem;
}

void Problem::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_settings()) {
      if (settings_ != NULL) settings_->::ProblemBuffers::ProblemSettings::Clear();
    }
    if (has_whiskers()) {
      if (whiskers_ != NULL) whiskers_->::RemyBuffers::WhiskerTree::Clear();
    }
  }
  configs_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Problem::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional .ProblemBuffers.ProblemSettings settings = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_settings()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_configs;
        break;
      }
      
      // repeated .RemyBuffers.NetConfig configs = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_configs:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_configs()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_configs;
        if (input->ExpectTag(26)) goto parse_whiskers;
        break;
      }
      
      // optional .RemyBuffers.WhiskerTree whiskers = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_whiskers:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_whiskers()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void Problem::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional .ProblemBuffers.ProblemSettings settings = 1;
  if (has_settings()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      1, this->settings(), output);
  }
  
  // repeated .RemyBuffers.NetConfig configs = 2;
  for (int i = 0; i < this->configs_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      2, this->configs(i), output);
  }
  
  // optional .RemyBuffers.WhiskerTree whiskers = 3;
  if (has_whiskers()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      3, this->whiskers(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* Problem::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional .ProblemBuffers.ProblemSettings settings = 1;
  if (has_settings()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        1, this->settings(), target);
  }
  
  // repeated .RemyBuffers.NetConfig configs = 2;
  for (int i = 0; i < this->configs_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        2, this->configs(i), target);
  }
  
  // optional .RemyBuffers.WhiskerTree whiskers = 3;
  if (has_whiskers()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        3, this->whiskers(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int Problem::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional .ProblemBuffers.ProblemSettings settings = 1;
    if (has_settings()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->settings());
    }
    
    // optional .RemyBuffers.WhiskerTree whiskers = 3;
    if (has_whiskers()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->whiskers());
    }
    
  }
  // repeated .RemyBuffers.NetConfig configs = 2;
  total_size += 1 * this->configs_size();
  for (int i = 0; i < this->configs_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->configs(i));
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Problem::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Problem* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Problem*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Problem::MergeFrom(const Problem& from) {
  GOOGLE_CHECK_NE(&from, this);
  configs_.MergeFrom(from.configs_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_settings()) {
      mutable_settings()->::ProblemBuffers::ProblemSettings::MergeFrom(from.settings());
    }
    if (from.has_whiskers()) {
      mutable_whiskers()->::RemyBuffers::WhiskerTree::MergeFrom(from.whiskers());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Problem::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Problem::CopyFrom(const Problem& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Problem::IsInitialized() const {
  
  return true;
}

void Problem::Swap(Problem* other) {
  if (other != this) {
    std::swap(settings_, other->settings_);
    configs_.Swap(&other->configs_);
    std::swap(whiskers_, other->whiskers_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Problem::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Problem_descriptor_;
  metadata.reflection = Problem_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int ProblemSettings::kPrngSeedFieldNumber;
const int ProblemSettings::kTickCountFieldNumber;
#endif  // !_MSC_VER

ProblemSettings::ProblemSettings()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void ProblemSettings::InitAsDefaultInstance() {
}

ProblemSettings::ProblemSettings(const ProblemSettings& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void ProblemSettings::SharedCtor() {
  _cached_size_ = 0;
  prng_seed_ = 0u;
  tick_count_ = 0u;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

ProblemSettings::~ProblemSettings() {
  SharedDtor();
}

void ProblemSettings::SharedDtor() {
  if (this != default_instance_) {
  }
}

void ProblemSettings::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* ProblemSettings::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return ProblemSettings_descriptor_;
}

const ProblemSettings& ProblemSettings::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_problem_2eproto();  return *default_instance_;
}

ProblemSettings* ProblemSettings::default_instance_ = NULL;

ProblemSettings* ProblemSettings::New() const {
  return new ProblemSettings;
}

void ProblemSettings::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    prng_seed_ = 0u;
    tick_count_ = 0u;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool ProblemSettings::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional uint32 prng_seed = 11;
      case 11: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &prng_seed_)));
          set_has_prng_seed();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(96)) goto parse_tick_count;
        break;
      }
      
      // optional uint32 tick_count = 12;
      case 12: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_tick_count:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &tick_count_)));
          set_has_tick_count();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void ProblemSettings::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional uint32 prng_seed = 11;
  if (has_prng_seed()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(11, this->prng_seed(), output);
  }
  
  // optional uint32 tick_count = 12;
  if (has_tick_count()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(12, this->tick_count(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* ProblemSettings::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional uint32 prng_seed = 11;
  if (has_prng_seed()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(11, this->prng_seed(), target);
  }
  
  // optional uint32 tick_count = 12;
  if (has_tick_count()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(12, this->tick_count(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int ProblemSettings::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional uint32 prng_seed = 11;
    if (has_prng_seed()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->prng_seed());
    }
    
    // optional uint32 tick_count = 12;
    if (has_tick_count()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->tick_count());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void ProblemSettings::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const ProblemSettings* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const ProblemSettings*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void ProblemSettings::MergeFrom(const ProblemSettings& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_prng_seed()) {
      set_prng_seed(from.prng_seed());
    }
    if (from.has_tick_count()) {
      set_tick_count(from.tick_count());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void ProblemSettings::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void ProblemSettings::CopyFrom(const ProblemSettings& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ProblemSettings::IsInitialized() const {
  
  return true;
}

void ProblemSettings::Swap(ProblemSettings* other) {
  if (other != this) {
    std::swap(prng_seed_, other->prng_seed_);
    std::swap(tick_count_, other->tick_count_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata ProblemSettings::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = ProblemSettings_descriptor_;
  metadata.reflection = ProblemSettings_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace ProblemBuffers

// @@protoc_insertion_point(global_scope)
