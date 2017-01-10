//===------- RPCUTils.h - Utilities for building RPC APIs -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Utilities to support construction of simple RPC APIs.
//
// The RPC utilities aim for ease of use (minimal conceptual overhead) for C++
// programmers, high performance, low memory overhead, and efficient use of the
// communications channel.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_RPCUTILS_H
#define LLVM_EXECUTIONENGINE_ORC_RPCUTILS_H

#include <map>
#include <thread>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/Orc/OrcError.h"
#include "llvm/ExecutionEngine/Orc/RPCSerialization.h"

#ifdef _MSC_VER
// concrt.h depends on eh.h for __uncaught_exception declaration
// even if we disable exceptions.
#include <eh.h>

// Disable warnings from ppltasks.h transitively included by <future>.
#pragma warning(push)
#pragma warning(disable : 4530)
#pragma warning(disable : 4062)
#endif

#include <future>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace llvm {
namespace orc {
namespace rpc {

template <typename DerivedFunc, typename FnT> class Function;

// RPC Function class.
// DerivedFunc should be a user defined class with a static 'getName()' method
// returning a const char* representing the function's name.
template <typename DerivedFunc, typename RetT, typename... ArgTs>
class Function<DerivedFunc, RetT(ArgTs...)> {
public:
  /// User defined function type.
  using Type = RetT(ArgTs...);

  /// Return type.
  using ReturnType = RetT;

  /// Returns the full function prototype as a string.
  static const char *getPrototype() {
    std::lock_guard<std::mutex> Lock(NameMutex);
    if (Name.empty())
      raw_string_ostream(Name)
          << RPCTypeName<RetT>::getName() << " " << DerivedFunc::getName()
          << "(" << llvm::orc::rpc::RPCTypeNameSequence<ArgTs...>() << ")";
    return Name.data();
  }

private:
  static std::mutex NameMutex;
  static std::string Name;
};

template <typename DerivedFunc, typename RetT, typename... ArgTs>
std::mutex Function<DerivedFunc, RetT(ArgTs...)>::NameMutex;

template <typename DerivedFunc, typename RetT, typename... ArgTs>
std::string Function<DerivedFunc, RetT(ArgTs...)>::Name;

/// Provides a typedef for a tuple containing the decayed argument types.
template <typename T> class FunctionArgsTuple;

template <typename RetT, typename... ArgTs>
class FunctionArgsTuple<RetT(ArgTs...)> {
public:
  using Type = std::tuple<typename std::decay<
      typename std::remove_reference<ArgTs>::type>::type...>;
};

/// Allocates RPC function ids during autonegotiation.
/// Specializations of this class must provide four members:
///
/// static T getInvalidId():
///   Should return a reserved id that will be used to represent missing
/// functions during autonegotiation.
///
/// static T getResponseId():
///   Should return a reserved id that will be used to send function responses
/// (return values).
///
/// static T getNegotiateId():
///   Should return a reserved id for the negotiate function, which will be used
/// to negotiate ids for user defined functions.
///
/// template <typename Func> T allocate():
///   Allocate a unique id for function Func.
template <typename T, typename = void> class RPCFunctionIdAllocator;

/// This specialization of RPCFunctionIdAllocator provides a default
/// implementation for integral types.
template <typename T>
class RPCFunctionIdAllocator<
    T, typename std::enable_if<std::is_integral<T>::value>::type> {
public:
  static T getInvalidId() { return T(0); }
  static T getResponseId() { return T(1); }
  static T getNegotiateId() { return T(2); }

  template <typename Func> T allocate() { return NextId++; }

private:
  T NextId = 3;
};

namespace detail {

// FIXME: Remove MSVCPError/MSVCPExpected once MSVC's future implementation
//        supports classes without default constructors.
#ifdef _MSC_VER

namespace msvc_hacks {

// Work around MSVC's future implementation's use of default constructors:
// A default constructed value in the promise will be overwritten when the
// real error is set - so the default constructed Error has to be checked
// already.
class MSVCPError : public Error {
public:
  MSVCPError() { (void)!!*this; }

  MSVCPError(MSVCPError &&Other) : Error(std::move(Other)) {}

  MSVCPError &operator=(MSVCPError Other) {
    Error::operator=(std::move(Other));
    return *this;
  }

  MSVCPError(Error Err) : Error(std::move(Err)) {}
};

// Work around MSVC's future implementation, similar to MSVCPError.
template <typename T> class MSVCPExpected : public Expected<T> {
public:
  MSVCPExpected()
      : Expected<T>(make_error<StringError>("", inconvertibleErrorCode())) {
    consumeError(this->takeError());
  }

  MSVCPExpected(MSVCPExpected &&Other) : Expected<T>(std::move(Other)) {}

  MSVCPExpected &operator=(MSVCPExpected &&Other) {
    Expected<T>::operator=(std::move(Other));
    return *this;
  }

  MSVCPExpected(Error Err) : Expected<T>(std::move(Err)) {}

  template <typename OtherT>
  MSVCPExpected(
      OtherT &&Val,
      typename std::enable_if<std::is_convertible<OtherT, T>::value>::type * =
          nullptr)
      : Expected<T>(std::move(Val)) {}

  template <class OtherT>
  MSVCPExpected(
      Expected<OtherT> &&Other,
      typename std::enable_if<std::is_convertible<OtherT, T>::value>::type * =
          nullptr)
      : Expected<T>(std::move(Other)) {}

  template <class OtherT>
  explicit MSVCPExpected(
      Expected<OtherT> &&Other,
      typename std::enable_if<!std::is_convertible<OtherT, T>::value>::type * =
          nullptr)
      : Expected<T>(std::move(Other)) {}
};

} // end namespace msvc_hacks

#endif // _MSC_VER

// ResultTraits provides typedefs and utilities specific to the return type
// of functions.
template <typename RetT> class ResultTraits {
public:
  // The return type wrapped in llvm::Expected.
  using ErrorReturnType = Expected<RetT>;

#ifdef _MSC_VER
  // The ErrorReturnType wrapped in a std::promise.
  using ReturnPromiseType = std::promise<msvc_hacks::MSVCPExpected<RetT>>;

  // The ErrorReturnType wrapped in a std::future.
  using ReturnFutureType = std::future<msvc_hacks::MSVCPExpected<RetT>>;
#else
  // The ErrorReturnType wrapped in a std::promise.
  using ReturnPromiseType = std::promise<ErrorReturnType>;

  // The ErrorReturnType wrapped in a std::future.
  using ReturnFutureType = std::future<ErrorReturnType>;
#endif

  // Create a 'blank' value of the ErrorReturnType, ready and safe to
  // overwrite.
  static ErrorReturnType createBlankErrorReturnValue() {
    return ErrorReturnType(RetT());
  }

  // Consume an abandoned ErrorReturnType.
  static void consumeAbandoned(ErrorReturnType RetOrErr) {
    consumeError(RetOrErr.takeError());
  }
};

// ResultTraits specialization for void functions.
template <> class ResultTraits<void> {
public:
  // For void functions, ErrorReturnType is llvm::Error.
  using ErrorReturnType = Error;

#ifdef _MSC_VER
  // The ErrorReturnType wrapped in a std::promise.
  using ReturnPromiseType = std::promise<msvc_hacks::MSVCPError>;

  // The ErrorReturnType wrapped in a std::future.
  using ReturnFutureType = std::future<msvc_hacks::MSVCPError>;
#else
  // The ErrorReturnType wrapped in a std::promise.
  using ReturnPromiseType = std::promise<ErrorReturnType>;

  // The ErrorReturnType wrapped in a std::future.
  using ReturnFutureType = std::future<ErrorReturnType>;
#endif

  // Create a 'blank' value of the ErrorReturnType, ready and safe to
  // overwrite.
  static ErrorReturnType createBlankErrorReturnValue() {
    return ErrorReturnType::success();
  }

  // Consume an abandoned ErrorReturnType.
  static void consumeAbandoned(ErrorReturnType Err) {
    consumeError(std::move(Err));
  }
};

// ResultTraits<Error> is equivalent to ResultTraits<void>. This allows
// handlers for void RPC functions to return either void (in which case they
// implicitly succeed) or Error (in which case their error return is
// propagated). See usage in HandlerTraits::runHandlerHelper.
template <> class ResultTraits<Error> : public ResultTraits<void> {};

// ResultTraits<Expected<T>> is equivalent to ResultTraits<T>. This allows
// handlers for RPC functions returning a T to return either a T (in which
// case they implicitly succeed) or Expected<T> (in which case their error
// return is propagated). See usage in HandlerTraits::runHandlerHelper.
template <typename RetT>
class ResultTraits<Expected<RetT>> : public ResultTraits<RetT> {};

// Send a response of the given wire return type (WireRetT) over the
// channel, with the given sequence number.
template <typename WireRetT, typename HandlerRetT, typename ChannelT,
          typename FunctionIdT, typename SequenceNumberT>
static Error respond(ChannelT &C, const FunctionIdT &ResponseId,
                     SequenceNumberT SeqNo, Expected<HandlerRetT> ResultOrErr) {
  // If this was an error bail out.
  // FIXME: Send an "error" message to the client if this is not a channel
  //        failure?
  if (auto Err = ResultOrErr.takeError())
    return Err;

  // Open the response message.
  if (auto Err = C.startSendMessage(ResponseId, SeqNo))
    return Err;

  // Serialize the result.
  if (auto Err =
          SerializationTraits<ChannelT, WireRetT, HandlerRetT>::serialize(
              C, *ResultOrErr))
    return Err;

  // Close the response message.
  return C.endSendMessage();
}

// Send an empty response message on the given channel to indicate that
// the handler ran.
template <typename WireRetT, typename ChannelT, typename FunctionIdT,
          typename SequenceNumberT>
static Error respond(ChannelT &C, const FunctionIdT &ResponseId,
                     SequenceNumberT SeqNo, Error Err) {
  if (Err)
    return Err;
  if (auto Err2 = C.startSendMessage(ResponseId, SeqNo))
    return Err2;
  return C.endSendMessage();
}

// Converts a given type to the equivalent error return type.
template <typename T> class WrappedHandlerReturn {
public:
  using Type = Expected<T>;
};

template <typename T> class WrappedHandlerReturn<Expected<T>> {
public:
  using Type = Expected<T>;
};

template <> class WrappedHandlerReturn<void> {
public:
  using Type = Error;
};

template <> class WrappedHandlerReturn<Error> {
public:
  using Type = Error;
};

template <> class WrappedHandlerReturn<ErrorSuccess> {
public:
  using Type = Error;
};

// This template class provides utilities related to RPC function handlers.
// The base case applies to non-function types (the template class is
// specialized for function types) and inherits from the appropriate
// speciilization for the given non-function type's call operator.
template <typename HandlerT>
class HandlerTraits : public HandlerTraits<decltype(
                          &std::remove_reference<HandlerT>::type::operator())> {
};

// Traits for handlers with a given function type.
template <typename RetT, typename... ArgTs>
class HandlerTraits<RetT(ArgTs...)> {
public:
  // Function type of the handler.
  using Type = RetT(ArgTs...);

  // Return type of the handler.
  using ReturnType = RetT;

  // A std::tuple wrapping the handler arguments.
  using ArgStorage = typename FunctionArgsTuple<RetT(ArgTs...)>::Type;

  // Call the given handler with the given arguments.
  template <typename HandlerT>
  static typename WrappedHandlerReturn<RetT>::Type
  unpackAndRun(HandlerT &Handler, ArgStorage &Args) {
    return unpackAndRunHelper(Handler, Args,
                              llvm::index_sequence_for<ArgTs...>());
  }

  // Call the given handler with the given arguments.
  template <typename HandlerT>
  static typename std::enable_if<
      std::is_void<typename HandlerTraits<HandlerT>::ReturnType>::value,
      Error>::type
  run(HandlerT &Handler, ArgTs &&... Args) {
    Handler(std::move(Args)...);
    return Error::success();
  }

  template <typename HandlerT>
  static typename std::enable_if<
      !std::is_void<typename HandlerTraits<HandlerT>::ReturnType>::value,
      typename HandlerTraits<HandlerT>::ReturnType>::type
  run(HandlerT &Handler, ArgTs... Args) {
    return Handler(std::move(Args)...);
  }

  // Serialize arguments to the channel.
  template <typename ChannelT, typename... CArgTs>
  static Error serializeArgs(ChannelT &C, const CArgTs... CArgs) {
    return SequenceSerialization<ChannelT, ArgTs...>::serialize(C, CArgs...);
  }

  // Deserialize arguments from the channel.
  template <typename ChannelT, typename... CArgTs>
  static Error deserializeArgs(ChannelT &C, std::tuple<CArgTs...> &Args) {
    return deserializeArgsHelper(C, Args,
                                 llvm::index_sequence_for<CArgTs...>());
  }

private:
  template <typename ChannelT, typename... CArgTs, size_t... Indexes>
  static Error deserializeArgsHelper(ChannelT &C, std::tuple<CArgTs...> &Args,
                                     llvm::index_sequence<Indexes...> _) {
    return SequenceSerialization<ChannelT, ArgTs...>::deserialize(
        C, std::get<Indexes>(Args)...);
  }

  template <typename HandlerT, size_t... Indexes>
  static typename WrappedHandlerReturn<
      typename HandlerTraits<HandlerT>::ReturnType>::Type
  unpackAndRunHelper(HandlerT &Handler, ArgStorage &Args,
                     llvm::index_sequence<Indexes...>) {
    return run(Handler, std::move(std::get<Indexes>(Args))...);
  }
};

// Handler traits for class methods (especially call operators for lambdas).
template <typename Class, typename RetT, typename... ArgTs>
class HandlerTraits<RetT (Class::*)(ArgTs...)>
    : public HandlerTraits<RetT(ArgTs...)> {};

// Handler traits for const class methods (especially call operators for
// lambdas).
template <typename Class, typename RetT, typename... ArgTs>
class HandlerTraits<RetT (Class::*)(ArgTs...) const>
    : public HandlerTraits<RetT(ArgTs...)> {};

// Utility to peel the Expected wrapper off a response handler error type.
template <typename HandlerT> class ResponseHandlerArg;

template <typename ArgT> class ResponseHandlerArg<Error(Expected<ArgT>)> {
public:
  using ArgType = Expected<ArgT>;
  using UnwrappedArgType = ArgT;
};

template <typename ArgT>
class ResponseHandlerArg<ErrorSuccess(Expected<ArgT>)> {
public:
  using ArgType = Expected<ArgT>;
  using UnwrappedArgType = ArgT;
};

template <> class ResponseHandlerArg<Error(Error)> {
public:
  using ArgType = Error;
};

template <> class ResponseHandlerArg<ErrorSuccess(Error)> {
public:
  using ArgType = Error;
};

// ResponseHandler represents a handler for a not-yet-received function call
// result.
template <typename ChannelT> class ResponseHandler {
public:
  virtual ~ResponseHandler() {}

  // Reads the function result off the wire and acts on it. The meaning of
  // "act" will depend on how this method is implemented in any given
  // ResponseHandler subclass but could, for example, mean running a
  // user-specified handler or setting a promise value.
  virtual Error handleResponse(ChannelT &C) = 0;

  // Abandons this outstanding result.
  virtual void abandon() = 0;

  // Create an error instance representing an abandoned response.
  static Error createAbandonedResponseError() {
    return orcError(OrcErrorCode::RPCResponseAbandoned);
  }
};

// ResponseHandler subclass for RPC functions with non-void returns.
template <typename ChannelT, typename FuncRetT, typename HandlerT>
class ResponseHandlerImpl : public ResponseHandler<ChannelT> {
public:
  ResponseHandlerImpl(HandlerT Handler) : Handler(std::move(Handler)) {}

  // Handle the result by deserializing it from the channel then passing it
  // to the user defined handler.
  Error handleResponse(ChannelT &C) override {
    using UnwrappedArgType = typename ResponseHandlerArg<
        typename HandlerTraits<HandlerT>::Type>::UnwrappedArgType;
    UnwrappedArgType Result;
    if (auto Err =
            SerializationTraits<ChannelT, FuncRetT,
                                UnwrappedArgType>::deserialize(C, Result))
      return Err;
    if (auto Err = C.endReceiveMessage())
      return Err;
    return Handler(Result);
  }

  // Abandon this response by calling the handler with an 'abandoned response'
  // error.
  void abandon() override {
    if (auto Err = Handler(this->createAbandonedResponseError())) {
      // Handlers should not fail when passed an abandoned response error.
      report_fatal_error(std::move(Err));
    }
  }

private:
  HandlerT Handler;
};

// ResponseHandler subclass for RPC functions with void returns.
template <typename ChannelT, typename HandlerT>
class ResponseHandlerImpl<ChannelT, void, HandlerT>
    : public ResponseHandler<ChannelT> {
public:
  ResponseHandlerImpl(HandlerT Handler) : Handler(std::move(Handler)) {}

  // Handle the result (no actual value, just a notification that the function
  // has completed on the remote end) by calling the user-defined handler with
  // Error::success().
  Error handleResponse(ChannelT &C) override {
    if (auto Err = C.endReceiveMessage())
      return Err;
    return Handler(Error::success());
  }

  // Abandon this response by calling the handler with an 'abandoned response'
  // error.
  void abandon() override {
    if (auto Err = Handler(this->createAbandonedResponseError())) {
      // Handlers should not fail when passed an abandoned response error.
      report_fatal_error(std::move(Err));
    }
  }

private:
  HandlerT Handler;
};

// Create a ResponseHandler from a given user handler.
template <typename ChannelT, typename FuncRetT, typename HandlerT>
std::unique_ptr<ResponseHandler<ChannelT>> createResponseHandler(HandlerT H) {
  return llvm::make_unique<ResponseHandlerImpl<ChannelT, FuncRetT, HandlerT>>(
      std::move(H));
}

// Helper for wrapping member functions up as functors. This is useful for
// installing methods as result handlers.
template <typename ClassT, typename RetT, typename... ArgTs>
class MemberFnWrapper {
public:
  using MethodT = RetT (ClassT::*)(ArgTs...);
  MemberFnWrapper(ClassT &Instance, MethodT Method)
      : Instance(Instance), Method(Method) {}
  RetT operator()(ArgTs &&... Args) {
    return (Instance.*Method)(std::move(Args)...);
  }

private:
  ClassT &Instance;
  MethodT Method;
};

// Helper that provides a Functor for deserializing arguments.
template <typename... ArgTs> class ReadArgs {
public:
  Error operator()() { return Error::success(); }
};

template <typename ArgT, typename... ArgTs>
class ReadArgs<ArgT, ArgTs...> : public ReadArgs<ArgTs...> {
public:
  ReadArgs(ArgT &Arg, ArgTs &... Args)
      : ReadArgs<ArgTs...>(Args...), Arg(Arg) {}

  Error operator()(ArgT &ArgVal, ArgTs &... ArgVals) {
    this->Arg = std::move(ArgVal);
    return ReadArgs<ArgTs...>::operator()(ArgVals...);
  }

private:
  ArgT &Arg;
};

// Manage sequence numbers.
template <typename SequenceNumberT> class SequenceNumberManager {
public:
  // Reset, making all sequence numbers available.
  void reset() {
    std::lock_guard<std::mutex> Lock(SeqNoLock);
    NextSequenceNumber = 0;
    FreeSequenceNumbers.clear();
  }

  // Get the next available sequence number. Will re-use numbers that have
  // been released.
  SequenceNumberT getSequenceNumber() {
    std::lock_guard<std::mutex> Lock(SeqNoLock);
    if (FreeSequenceNumbers.empty())
      return NextSequenceNumber++;
    auto SequenceNumber = FreeSequenceNumbers.back();
    FreeSequenceNumbers.pop_back();
    return SequenceNumber;
  }

  // Release a sequence number, making it available for re-use.
  void releaseSequenceNumber(SequenceNumberT SequenceNumber) {
    std::lock_guard<std::mutex> Lock(SeqNoLock);
    FreeSequenceNumbers.push_back(SequenceNumber);
  }

private:
  std::mutex SeqNoLock;
  SequenceNumberT NextSequenceNumber = 0;
  std::vector<SequenceNumberT> FreeSequenceNumbers;
};

// Checks that predicate P holds for each corresponding pair of type arguments
// from T1 and T2 tuple.
template <template <class, class> class P, typename T1Tuple, typename T2Tuple>
class RPCArgTypeCheckHelper;

template <template <class, class> class P>
class RPCArgTypeCheckHelper<P, std::tuple<>, std::tuple<>> {
public:
  static const bool value = true;
};

template <template <class, class> class P, typename T, typename... Ts,
          typename U, typename... Us>
class RPCArgTypeCheckHelper<P, std::tuple<T, Ts...>, std::tuple<U, Us...>> {
public:
  static const bool value =
      P<T, U>::value &&
      RPCArgTypeCheckHelper<P, std::tuple<Ts...>, std::tuple<Us...>>::value;
};

template <template <class, class> class P, typename T1Sig, typename T2Sig>
class RPCArgTypeCheck {
public:
  using T1Tuple = typename FunctionArgsTuple<T1Sig>::Type;
  using T2Tuple = typename FunctionArgsTuple<T2Sig>::Type;

  static_assert(std::tuple_size<T1Tuple>::value >=
                    std::tuple_size<T2Tuple>::value,
                "Too many arguments to RPC call");
  static_assert(std::tuple_size<T1Tuple>::value <=
                    std::tuple_size<T2Tuple>::value,
                "Too few arguments to RPC call");

  static const bool value = RPCArgTypeCheckHelper<P, T1Tuple, T2Tuple>::value;
};

template <typename ChannelT, typename WireT, typename ConcreteT>
class CanSerialize {
private:
  using S = SerializationTraits<ChannelT, WireT, ConcreteT>;

  template <typename T>
  static std::true_type
  check(typename std::enable_if<
        std::is_same<decltype(T::serialize(std::declval<ChannelT &>(),
                                           std::declval<const ConcreteT &>())),
                     Error>::value,
        void *>::type);

  template <typename> static std::false_type check(...);

public:
  static const bool value = decltype(check<S>(0))::value;
};

template <typename ChannelT, typename WireT, typename ConcreteT>
class CanDeserialize {
private:
  using S = SerializationTraits<ChannelT, WireT, ConcreteT>;

  template <typename T>
  static std::true_type
  check(typename std::enable_if<
        std::is_same<decltype(T::deserialize(std::declval<ChannelT &>(),
                                             std::declval<ConcreteT &>())),
                     Error>::value,
        void *>::type);

  template <typename> static std::false_type check(...);

public:
  static const bool value = decltype(check<S>(0))::value;
};

/// Contains primitive utilities for defining, calling and handling calls to
/// remote procedures. ChannelT is a bidirectional stream conforming to the
/// RPCChannel interface (see RPCChannel.h), FunctionIdT is a procedure
/// identifier type that must be serializable on ChannelT, and SequenceNumberT
/// is an integral type that will be used to number in-flight function calls.
///
/// These utilities support the construction of very primitive RPC utilities.
/// Their intent is to ensure correct serialization and deserialization of
/// procedure arguments, and to keep the client and server's view of the API in
/// sync.
template <typename ImplT, typename ChannelT, typename FunctionIdT,
          typename SequenceNumberT>
class RPCEndpointBase {
protected:
  class OrcRPCInvalid : public Function<OrcRPCInvalid, void()> {
  public:
    static const char *getName() { return "__orc_rpc$invalid"; }
  };

  class OrcRPCResponse : public Function<OrcRPCResponse, void()> {
  public:
    static const char *getName() { return "__orc_rpc$response"; }
  };

  class OrcRPCNegotiate
      : public Function<OrcRPCNegotiate, FunctionIdT(std::string)> {
  public:
    static const char *getName() { return "__orc_rpc$negotiate"; }
  };

  // Helper predicate for testing for the presence of SerializeTraits
  // serializers.
  template <typename WireT, typename ConcreteT>
  class CanSerializeCheck : detail::CanSerialize<ChannelT, WireT, ConcreteT> {
  public:
    using detail::CanSerialize<ChannelT, WireT, ConcreteT>::value;

    static_assert(value, "Missing serializer for argument (Can't serialize the "
                         "first template type argument of CanSerializeCheck "
                         "from the second)");
  };

  // Helper predicate for testing for the presence of SerializeTraits
  // deserializers.
  template <typename WireT, typename ConcreteT>
  class CanDeserializeCheck
      : detail::CanDeserialize<ChannelT, WireT, ConcreteT> {
  public:
    using detail::CanDeserialize<ChannelT, WireT, ConcreteT>::value;

    static_assert(value, "Missing deserializer for argument (Can't deserialize "
                         "the second template type argument of "
                         "CanDeserializeCheck from the first)");
  };

public:
  /// Construct an RPC instance on a channel.
  RPCEndpointBase(ChannelT &C, bool LazyAutoNegotiation)
      : C(C), LazyAutoNegotiation(LazyAutoNegotiation) {
    // Hold ResponseId in a special variable, since we expect Response to be
    // called relatively frequently, and want to avoid the map lookup.
    ResponseId = FnIdAllocator.getResponseId();
    RemoteFunctionIds[OrcRPCResponse::getPrototype()] = ResponseId;

    // Register the negotiate function id and handler.
    auto NegotiateId = FnIdAllocator.getNegotiateId();
    RemoteFunctionIds[OrcRPCNegotiate::getPrototype()] = NegotiateId;
    Handlers[NegotiateId] = wrapHandler<OrcRPCNegotiate>(
        [this](const std::string &Name) { return handleNegotiate(Name); },
        LaunchPolicy());
  }

  /// Append a call Func, does not call send on the channel.
  /// The first argument specifies a user-defined handler to be run when the
  /// function returns. The handler should take an Expected<Func::ReturnType>,
  /// or an Error (if Func::ReturnType is void). The handler will be called
  /// with an error if the return value is abandoned due to a channel error.
  template <typename Func, typename HandlerT, typename... ArgTs>
  Error appendCallAsync(HandlerT Handler, const ArgTs &... Args) {

    static_assert(
        detail::RPCArgTypeCheck<CanSerializeCheck, typename Func::Type,
                                void(ArgTs...)>::value,
        "");

    // Look up the function ID.
    FunctionIdT FnId;
    if (auto FnIdOrErr = getRemoteFunctionId<Func>())
      FnId = *FnIdOrErr;
    else {
      // This isn't a channel error so we don't want to abandon other pending
      // responses, but we still need to run the user handler with an error to
      // let them know the call failed.
      if (auto Err = Handler(orcError(OrcErrorCode::UnknownRPCFunction)))
        report_fatal_error(std::move(Err));
      return FnIdOrErr.takeError();
    }

    SequenceNumberT SeqNo; // initialized in locked scope below.
    {
      // Lock the pending responses map and sequence number manager.
      std::lock_guard<std::mutex> Lock(ResponsesMutex);

      // Allocate a sequence number.
      SeqNo = SequenceNumberMgr.getSequenceNumber();
      assert(!PendingResponses.count(SeqNo) &&
             "Sequence number already allocated");

      // Install the user handler.
      PendingResponses[SeqNo] =
        detail::createResponseHandler<ChannelT, typename Func::ReturnType>(
            std::move(Handler));
    }

    // Open the function call message.
    if (auto Err = C.startSendMessage(FnId, SeqNo)) {
      abandonPendingResponses();
      return joinErrors(std::move(Err), C.endSendMessage());
    }

    // Serialize the call arguments.
    if (auto Err = detail::HandlerTraits<typename Func::Type>::serializeArgs(
            C, Args...)) {
      abandonPendingResponses();
      return joinErrors(std::move(Err), C.endSendMessage());
    }

    // Close the function call messagee.
    if (auto Err = C.endSendMessage()) {
      abandonPendingResponses();
      return std::move(Err);
    }

    return Error::success();
  }

  Error sendAppendedCalls() { return C.send(); };

  template <typename Func, typename HandlerT, typename... ArgTs>
  Error callAsync(HandlerT Handler, const ArgTs &... Args) {
    if (auto Err = appendCallAsync<Func>(std::move(Handler), Args...))
      return Err;
    return C.send();
  }

  /// Handle one incoming call.
  Error handleOne() {
    FunctionIdT FnId;
    SequenceNumberT SeqNo;
    if (auto Err = C.startReceiveMessage(FnId, SeqNo))
      return Err;
    if (FnId == ResponseId)
      return handleResponse(SeqNo);
    auto I = Handlers.find(FnId);
    if (I != Handlers.end())
      return I->second(C, SeqNo);

    // else: No handler found. Report error to client?
    return orcError(OrcErrorCode::UnexpectedRPCCall);
  }

  /// Helper for handling setter procedures - this method returns a functor that
  /// sets the variables referred to by Args... to values deserialized from the
  /// channel.
  /// E.g.
  ///
  ///   typedef Function<0, bool, int> Func1;
  ///
  ///   ...
  ///   bool B;
  ///   int I;
  ///   if (auto Err = expect<Func1>(Channel, readArgs(B, I)))
  ///     /* Handle Args */ ;
  ///
  template <typename... ArgTs>
  static detail::ReadArgs<ArgTs...> readArgs(ArgTs &... Args) {
    return detail::ReadArgs<ArgTs...>(Args...);
  }

  /// Abandon all outstanding result handlers.
  ///
  /// This will call all currently registered result handlers to receive an
  /// "abandoned" error as their argument. This is used internally by the RPC
  /// in error situations, but can also be called directly by clients who are
  /// disconnecting from the remote and don't or can't expect responses to their
  /// outstanding calls. (Especially for outstanding blocking calls, calling
  /// this function may be necessary to avoid dead threads).
  void abandonPendingResponses() {
    // Lock the pending responses map and sequence number manager.
    std::lock_guard<std::mutex> Lock(ResponsesMutex);

    for (auto &KV : PendingResponses)
      KV.second->abandon();
    PendingResponses.clear();
    SequenceNumberMgr.reset();
  }

protected:
  // The LaunchPolicy type allows a launch policy to be specified when adding
  // a function handler. See addHandlerImpl.
  using LaunchPolicy = std::function<Error(std::function<Error()>)>;

  FunctionIdT getInvalidFunctionId() const {
    return FnIdAllocator.getInvalidId();
  }

  /// Add the given handler to the handler map and make it available for
  /// autonegotiation and execution.
  template <typename Func, typename HandlerT>
  void addHandlerImpl(HandlerT Handler, LaunchPolicy Launch) {

    static_assert(detail::RPCArgTypeCheck<
                      CanDeserializeCheck, typename Func::Type,
                      typename detail::HandlerTraits<HandlerT>::Type>::value,
                  "");

    FunctionIdT NewFnId = FnIdAllocator.template allocate<Func>();
    LocalFunctionIds[Func::getPrototype()] = NewFnId;
    Handlers[NewFnId] =
        wrapHandler<Func>(std::move(Handler), std::move(Launch));
  }

  Error handleResponse(SequenceNumberT SeqNo) {
    using Handler = typename decltype(PendingResponses)::mapped_type;
    Handler PRHandler;

    {
      // Lock the pending responses map and sequence number manager.
      std::unique_lock<std::mutex> Lock(ResponsesMutex);
      auto I = PendingResponses.find(SeqNo);

      if (I != PendingResponses.end()) {
        PRHandler = std::move(I->second);
        PendingResponses.erase(I);
        SequenceNumberMgr.releaseSequenceNumber(SeqNo);
      } else {
        // Unlock the pending results map to prevent recursive lock.
        Lock.unlock();
        abandonPendingResponses();
        return orcError(OrcErrorCode::UnexpectedRPCResponse);
      }
    }

    assert(PRHandler &&
           "If we didn't find a response handler we should have bailed out");

    if (auto Err = PRHandler->handleResponse(C)) {
      abandonPendingResponses();
      return Err;
    }

    return Error::success();
  }

  FunctionIdT handleNegotiate(const std::string &Name) {
    auto I = LocalFunctionIds.find(Name);
    if (I == LocalFunctionIds.end())
      return getInvalidFunctionId();
    return I->second;
  }

  // Find the remote FunctionId for the given function, which must be in the
  // RemoteFunctionIds map.
  template <typename Func> Expected<FunctionIdT> getRemoteFunctionId() {
    // Try to find the id for the given function.
    auto I = RemoteFunctionIds.find(Func::getPrototype());

    // If we have it in the map, return it.
    if (I != RemoteFunctionIds.end())
      return I->second;

    // Otherwise, if we have auto-negotiation enabled, try to negotiate it.
    if (LazyAutoNegotiation) {
      auto &Impl = static_cast<ImplT &>(*this);
      if (auto RemoteIdOrErr =
              Impl.template callB<OrcRPCNegotiate>(Func::getPrototype())) {
        auto &RemoteId = *RemoteIdOrErr;

        // If autonegotiation indicates that the remote end doesn't support this
        // function, return an unknown function error.
        if (RemoteId == getInvalidFunctionId())
          return orcError(OrcErrorCode::UnknownRPCFunction);

        // Autonegotiation succeeded and returned a valid id. Update the map and
        // return the id.
        RemoteFunctionIds[Func::getPrototype()] = RemoteId;
        return RemoteId;
      } else {
        // Autonegotiation failed. Return the error.
        return RemoteIdOrErr.takeError();
      }
    }

    // No key was available in the map and autonegotiation wasn't enabled.
    // Return an unknown function error.
    return orcError(OrcErrorCode::UnknownRPCFunction);
  }

  using WrappedHandlerFn = std::function<Error(ChannelT &, SequenceNumberT)>;

  // Wrap the given user handler in the necessary argument-deserialization code,
  // result-serialization code, and call to the launch policy (if present).
  template <typename Func, typename HandlerT>
  WrappedHandlerFn wrapHandler(HandlerT Handler, LaunchPolicy Launch) {
    return [this, Handler, Launch](ChannelT &Channel,
                                   SequenceNumberT SeqNo) mutable -> Error {
      // Start by deserializing the arguments.
      auto Args = std::make_shared<
          typename detail::HandlerTraits<HandlerT>::ArgStorage>();
      if (auto Err =
              detail::HandlerTraits<typename Func::Type>::deserializeArgs(
                  Channel, *Args))
        return Err;

      // GCC 4.7 and 4.8 incorrectly issue a -Wunused-but-set-variable warning
      // for RPCArgs. Void cast RPCArgs to work around this for now.
      // FIXME: Remove this workaround once we can assume a working GCC version.
      (void)Args;

      // End receieve message, unlocking the channel for reading.
      if (auto Err = Channel.endReceiveMessage())
        return Err;

      // Build the handler/responder.
      auto Responder = [this, Handler, Args, &Channel,
                        SeqNo]() mutable -> Error {
        using HTraits = detail::HandlerTraits<HandlerT>;
        using FuncReturn = typename Func::ReturnType;
        return detail::respond<FuncReturn>(
            Channel, ResponseId, SeqNo, HTraits::unpackAndRun(Handler, *Args));
      };

      // If there is an explicit launch policy then use it to launch the
      // handler.
      if (Launch)
        return Launch(std::move(Responder));

      // Otherwise run the handler on the listener thread.
      return Responder();
    };
  }

  ChannelT &C;

  bool LazyAutoNegotiation;

  RPCFunctionIdAllocator<FunctionIdT> FnIdAllocator;

  FunctionIdT ResponseId;
  std::map<std::string, FunctionIdT> LocalFunctionIds;
  std::map<const char *, FunctionIdT> RemoteFunctionIds;

  std::map<FunctionIdT, WrappedHandlerFn> Handlers;

  std::mutex ResponsesMutex;
  detail::SequenceNumberManager<SequenceNumberT> SequenceNumberMgr;
  std::map<SequenceNumberT, std::unique_ptr<detail::ResponseHandler<ChannelT>>>
      PendingResponses;
};

} // end namespace detail

template <typename ChannelT, typename FunctionIdT = uint32_t,
          typename SequenceNumberT = uint32_t>
class MultiThreadedRPCEndpoint
    : public detail::RPCEndpointBase<
          MultiThreadedRPCEndpoint<ChannelT, FunctionIdT, SequenceNumberT>,
          ChannelT, FunctionIdT, SequenceNumberT> {
private:
  using BaseClass =
      detail::RPCEndpointBase<
        MultiThreadedRPCEndpoint<ChannelT, FunctionIdT, SequenceNumberT>,
        ChannelT, FunctionIdT, SequenceNumberT>;

public:
  MultiThreadedRPCEndpoint(ChannelT &C, bool LazyAutoNegotiation)
      : BaseClass(C, LazyAutoNegotiation) {}

  /// The LaunchPolicy type allows a launch policy to be specified when adding
  /// a function handler. See addHandler.
  using LaunchPolicy = typename BaseClass::LaunchPolicy;

  /// Add a handler for the given RPC function.
  /// This installs the given handler functor for the given RPC Function, and
  /// makes the RPC function available for negotiation/calling from the remote.
  ///
  /// The optional LaunchPolicy argument can be used to control how the handler
  /// is run when called:
  ///
  /// * If no LaunchPolicy is given, the handler code will be run on the RPC
  ///   handler thread that is reading from the channel. This handler cannot
  ///   make blocking RPC calls (since it would be blocking the thread used to
  ///   get the result), but can make non-blocking calls.
  ///
  /// * If a LaunchPolicy is given, the user's handler will be wrapped in a
  ///   call to serialize and send the result, and the resulting functor (with
  ///   type 'Error()' will be passed to the LaunchPolicy. The user can then
  ///   choose to add the wrapped handler to a work queue, spawn a new thread,
  ///   or anything else.
  template <typename Func, typename HandlerT>
  void addHandler(HandlerT Handler, LaunchPolicy Launch = LaunchPolicy()) {
    return this->template addHandlerImpl<Func>(std::move(Handler),
                                               std::move(Launch));
  }

  /// Add a class-method as a handler.
  template <typename Func, typename ClassT, typename RetT, typename... ArgTs>
  void addHandler(ClassT &Object, RetT (ClassT::*Method)(ArgTs...),
                  LaunchPolicy Launch = LaunchPolicy()) {
    addHandler<Func>(
      detail::MemberFnWrapper<ClassT, RetT, ArgTs...>(Object, Method),
      Launch);
  }

  /// Negotiate a function id for Func with the other end of the channel.
  template <typename Func> Error negotiateFunction(bool Retry = false) {
    using OrcRPCNegotiate = typename BaseClass::OrcRPCNegotiate;

    // Check if we already have a function id...
    auto I = this->RemoteFunctionIds.find(Func::getPrototype());
    if (I != this->RemoteFunctionIds.end()) {
      // If it's valid there's nothing left to do.
      if (I->second != this->getInvalidFunctionId())
        return Error::success();
      // If it's invalid and we can't re-attempt negotiation, throw an error.
      if (!Retry)
        return orcError(OrcErrorCode::UnknownRPCFunction);
    }

    // We don't have a function id for Func yet, call the remote to try to
    // negotiate one.
    if (auto RemoteIdOrErr = callB<OrcRPCNegotiate>(Func::getPrototype())) {
      this->RemoteFunctionIds[Func::getPrototype()] = *RemoteIdOrErr;
      if (*RemoteIdOrErr == this->getInvalidFunctionId())
        return orcError(OrcErrorCode::UnknownRPCFunction);
      return Error::success();
    } else
      return RemoteIdOrErr.takeError();
  }

  /// Return type for non-blocking call primitives.
  template <typename Func>
  using NonBlockingCallResult = typename detail::ResultTraits<
      typename Func::ReturnType>::ReturnFutureType;

  /// Call Func on Channel C. Does not block, does not call send. Returns a pair
  /// of a future result and the sequence number assigned to the result.
  ///
  /// This utility function is primarily used for single-threaded mode support,
  /// where the sequence number can be used to wait for the corresponding
  /// result. In multi-threaded mode the appendCallNB method, which does not
  /// return the sequence numeber, should be preferred.
  template <typename Func, typename... ArgTs>
  Expected<NonBlockingCallResult<Func>> appendCallNB(const ArgTs &... Args) {
    using RTraits = detail::ResultTraits<typename Func::ReturnType>;
    using ErrorReturn = typename RTraits::ErrorReturnType;
    using ErrorReturnPromise = typename RTraits::ReturnPromiseType;

    // FIXME: Stack allocate and move this into the handler once LLVM builds
    //        with C++14.
    auto Promise = std::make_shared<ErrorReturnPromise>();
    auto FutureResult = Promise->get_future();

    if (auto Err = this->template appendCallAsync<Func>(
            [Promise](ErrorReturn RetOrErr) {
              Promise->set_value(std::move(RetOrErr));
              return Error::success();
            },
            Args...)) {
      this->abandonPendingResponses();
      RTraits::consumeAbandoned(FutureResult.get());
      return std::move(Err);
    }
    return std::move(FutureResult);
  }

  /// The same as appendCallNBWithSeq, except that it calls C.send() to
  /// flush the channel after serializing the call.
  template <typename Func, typename... ArgTs>
  Expected<NonBlockingCallResult<Func>> callNB(const ArgTs &... Args) {
    auto Result = appendCallNB<Func>(Args...);
    if (!Result)
      return Result;
    if (auto Err = this->C.send()) {
      this->abandonPendingResponses();
      detail::ResultTraits<typename Func::ReturnType>::consumeAbandoned(
          std::move(Result->get()));
      return std::move(Err);
    }
    return Result;
  }

  /// Call Func on Channel C. Blocks waiting for a result. Returns an Error
  /// for void functions or an Expected<T> for functions returning a T.
  ///
  /// This function is for use in threaded code where another thread is
  /// handling responses and incoming calls.
  template <typename Func, typename... ArgTs,
            typename AltRetT = typename Func::ReturnType>
  typename detail::ResultTraits<AltRetT>::ErrorReturnType
  callB(const ArgTs &... Args) {
    if (auto FutureResOrErr = callNB<Func>(Args...)) {
      if (auto Err = this->C.send()) {
        this->abandonPendingResponses();
        detail::ResultTraits<typename Func::ReturnType>::consumeAbandoned(
            std::move(FutureResOrErr->get()));
        return std::move(Err);
      }
      return FutureResOrErr->get();
    } else
      return FutureResOrErr.takeError();
  }

  /// Handle incoming RPC calls.
  Error handlerLoop() {
    while (true)
      if (auto Err = this->handleOne())
        return Err;
    return Error::success();
  }
};

template <typename ChannelT, typename FunctionIdT = uint32_t,
          typename SequenceNumberT = uint32_t>
class SingleThreadedRPCEndpoint
    : public detail::RPCEndpointBase<
          SingleThreadedRPCEndpoint<ChannelT, FunctionIdT, SequenceNumberT>,
          ChannelT, FunctionIdT, SequenceNumberT> {
private:
  using BaseClass =
      detail::RPCEndpointBase<
        SingleThreadedRPCEndpoint<ChannelT, FunctionIdT, SequenceNumberT>,
        ChannelT, FunctionIdT, SequenceNumberT>;

  using LaunchPolicy = typename BaseClass::LaunchPolicy;

public:
  SingleThreadedRPCEndpoint(ChannelT &C, bool LazyAutoNegotiation)
      : BaseClass(C, LazyAutoNegotiation) {}

  template <typename Func, typename HandlerT>
  void addHandler(HandlerT Handler) {
    return this->template addHandlerImpl<Func>(std::move(Handler),
                                               LaunchPolicy());
  }

  template <typename Func, typename ClassT, typename RetT, typename... ArgTs>
  void addHandler(ClassT &Object, RetT (ClassT::*Method)(ArgTs...)) {
    addHandler<Func>(
        detail::MemberFnWrapper<ClassT, RetT, ArgTs...>(Object, Method));
  }

  /// Negotiate a function id for Func with the other end of the channel.
  template <typename Func> Error negotiateFunction(bool Retry = false) {
    using OrcRPCNegotiate = typename BaseClass::OrcRPCNegotiate;

    // Check if we already have a function id...
    auto I = this->RemoteFunctionIds.find(Func::getPrototype());
    if (I != this->RemoteFunctionIds.end()) {
      // If it's valid there's nothing left to do.
      if (I->second != this->getInvalidFunctionId())
        return Error::success();
      // If it's invalid and we can't re-attempt negotiation, throw an error.
      if (!Retry)
        return orcError(OrcErrorCode::UnknownRPCFunction);
    }

    // We don't have a function id for Func yet, call the remote to try to
    // negotiate one.
    if (auto RemoteIdOrErr = callB<OrcRPCNegotiate>(Func::getPrototype())) {
      this->RemoteFunctionIds[Func::getPrototype()] = *RemoteIdOrErr;
      if (*RemoteIdOrErr == this->getInvalidFunctionId())
        return orcError(OrcErrorCode::UnknownRPCFunction);
      return Error::success();
    } else
      return RemoteIdOrErr.takeError();
  }

  template <typename Func, typename... ArgTs,
            typename AltRetT = typename Func::ReturnType>
  typename detail::ResultTraits<AltRetT>::ErrorReturnType
  callB(const ArgTs &... Args) {
    bool ReceivedResponse = false;
    using ResultType = typename detail::ResultTraits<AltRetT>::ErrorReturnType;
    auto Result = detail::ResultTraits<AltRetT>::createBlankErrorReturnValue();

    // We have to 'Check' result (which we know is in a success state at this
    // point) so that it can be overwritten in the async handler.
    (void)!!Result;

    if (auto Err = this->template appendCallAsync<Func>(
            [&](ResultType R) {
              Result = std::move(R);
              ReceivedResponse = true;
              return Error::success();
            },
            Args...)) {
      this->abandonPendingResponses();
      detail::ResultTraits<typename Func::ReturnType>::consumeAbandoned(
          std::move(Result));
      return std::move(Err);
    }

    while (!ReceivedResponse) {
      if (auto Err = this->handleOne()) {
        this->abandonPendingResponses();
        detail::ResultTraits<typename Func::ReturnType>::consumeAbandoned(
            std::move(Result));
        return std::move(Err);
      }
    }

    return Result;
  }
};

/// \brief Allows a set of asynchrounous calls to be dispatched, and then
///        waited on as a group.
template <typename RPCClass> class ParallelCallGroup {
public:

  /// \brief Construct a parallel call group for the given RPC.
  ParallelCallGroup(RPCClass &RPC) : RPC(RPC), NumOutstandingCalls(0) {}

  ParallelCallGroup(const ParallelCallGroup &) = delete;
  ParallelCallGroup &operator=(const ParallelCallGroup &) = delete;

  /// \brief Make as asynchronous call.
  ///
  /// Does not issue a send call to the RPC's channel. The channel may use this
  /// to batch up subsequent calls. A send will automatically be sent when wait
  /// is called.
  template <typename Func, typename HandlerT, typename... ArgTs>
  Error appendCall(HandlerT Handler, const ArgTs &... Args) {
    // Increment the count of outstanding calls. This has to happen before
    // we invoke the call, as the handler may (depending on scheduling)
    // be run immediately on another thread, and we don't want the decrement
    // in the wrapped handler below to run before the increment.
    {
      std::unique_lock<std::mutex> Lock(M);
      ++NumOutstandingCalls;
    }

    // Wrap the user handler in a lambda that will decrement the
    // outstanding calls count, then poke the condition variable.
    using ArgType = typename detail::ResponseHandlerArg<
        typename detail::HandlerTraits<HandlerT>::Type>::ArgType;
    // FIXME: Move handler into wrapped handler once we have C++14.
    auto WrappedHandler = [this, Handler](ArgType Arg) {
      auto Err = Handler(std::move(Arg));
      std::unique_lock<std::mutex> Lock(M);
      --NumOutstandingCalls;
      CV.notify_all();
      return Err;
    };

    return RPC.template appendCallAsync<Func>(std::move(WrappedHandler),
                                              Args...);
  }

  /// \brief Make an asynchronous call.
  ///
  /// The same as appendCall, but also calls send on the channel immediately.
  /// Prefer appendCall if you are about to issue a "wait" call shortly, as
  /// this may allow the channel to better batch the calls.
  template <typename Func, typename HandlerT, typename... ArgTs>
  Error call(HandlerT Handler, const ArgTs &... Args) {
    if (auto Err = appendCall(std::move(Handler), Args...))
      return Err;
    return RPC.sendAppendedCalls();
  }

  /// \brief Blocks until all calls have been completed and their return value
  ///        handlers run.
  Error wait() {
    if (auto Err = RPC.sendAppendedCalls())
      return Err;
    std::unique_lock<std::mutex> Lock(M);
    while (NumOutstandingCalls > 0)
      CV.wait(Lock);
    return Error::success();
  }

private:
  RPCClass &RPC;
  std::mutex M;
  std::condition_variable CV;
  uint32_t NumOutstandingCalls;
};

/// @brief Convenience class for grouping RPC Functions into APIs that can be
///        negotiated as a block.
///
template <typename... Funcs>
class APICalls {
public:

  /// @brief Test whether this API contains Function F.
  template <typename F>
  class Contains {
  public:
    static const bool value = false;
  };

  /// @brief Negotiate all functions in this API.
  template <typename RPCEndpoint>
  static Error negotiate(RPCEndpoint &R) {
    return Error::success();
  }
};

template <typename Func, typename... Funcs>
class APICalls<Func, Funcs...> {
public:

  template <typename F>
  class Contains {
  public:
    static const bool value = std::is_same<F, Func>::value |
                              APICalls<Funcs...>::template Contains<F>::value;
  };

  template <typename RPCEndpoint>
  static Error negotiate(RPCEndpoint &R) {
    if (auto Err = R.template negotiateFunction<Func>())
      return Err;
    return APICalls<Funcs...>::negotiate(R);
  }

};

template <typename... InnerFuncs, typename... Funcs>
class APICalls<APICalls<InnerFuncs...>, Funcs...> {
public:

  template <typename F>
  class Contains {
  public:
    static const bool value =
      APICalls<InnerFuncs...>::template Contains<F>::value |
      APICalls<Funcs...>::template Contains<F>::value;
  };

  template <typename RPCEndpoint>
  static Error negotiate(RPCEndpoint &R) {
    if (auto Err = APICalls<InnerFuncs...>::negotiate(R))
      return Err;
    return APICalls<Funcs...>::negotiate(R);
  }

};

} // end namespace rpc
} // end namespace orc
} // end namespace llvm

#endif
