// RUN: %clang_cc1  -triple x86_64-apple-darwin11 -fsyntax-only -verify %s -Wno-objc-root-class

// Mark this protocol as requiring all of its methods and properties
// to be explicitly implemented in the adopting class.
__attribute__((objc_protocol_requires_explicit_implementation))
@protocol Protocol
- (void) theBestOfTimes; // expected-note {{method 'theBestOfTimes' declared here}}
@property (readonly) id theWorstOfTimes;
@end

// In this example, ClassA adopts the protocol.  We won't
// provide the implementation here, but this protocol will
// be adopted later by a subclass.
@interface ClassA <Protocol>
- (void) theBestOfTimes;
@property (readonly) id theWorstOfTimes;
@end

// This class subclasses ClassA (which adopts 'Protocol'),
// but does not provide the needed implementation.
@interface ClassB : ClassA <Protocol>
@end

@implementation ClassB // expected-warning {{method 'theBestOfTimes' in protocol 'Protocol' not implemented}}
@end

// Test that inherited protocols do not get the explicit conformance requirement.
@protocol Inherited
- (void) fairIsFoul;
@end

__attribute__((objc_protocol_requires_explicit_implementation))
@protocol Derived <Inherited>
- (void) foulIsFair; // expected-note {{method 'foulIsFair' declared here}}
@end

@interface ClassC <Inherited>
@end

@interface ClassD : ClassC <Derived>
@end

@implementation ClassD // expected-warning {{method 'foulIsFair' in protocol 'Derived' not implemented}}
@end

// Test that the attribute is used correctly.
__attribute__((objc_protocol_requires_explicit_implementation(1+2))) // expected-error {{attribute takes no arguments}}
@protocol AnotherProtocol @end

// Cannot put the attribute on classes or other non-protocol declarations.
__attribute__((objc_protocol_requires_explicit_implementation)) // expected-error {{attribute only applies to Objective-C protocols}}
@interface AnotherClass @end

__attribute__((objc_protocol_requires_explicit_implementation)) // expected-error {{attribute only applies to Objective-C protocols}}
int x;

// Test that inherited protocols with the attribute
// are treated properly.
__attribute__((objc_protocol_requires_explicit_implementation))
@protocol ProtocolA
@required
- (void)rlyeh; // expected-note 2 {{method 'rlyeh' declared here}}
- (void)innsmouth; // expected-note 2 {{method 'innsmouth' declared here}}
@end

@protocol ProtocolB <ProtocolA>
@required
- (void)dunwich;
- (void)innsmouth; // expected-note {{method 'innsmouth' declared here}}
@end

__attribute__((objc_protocol_requires_explicit_implementation))
@protocol ProtocolB_Explicit <ProtocolA>
@required
- (void)dunwich;
- (void)innsmouth; // expected-note 2 {{method 'innsmouth' declared here}}
@end

@protocol ProtocolC
@required
- (void)rlyeh;
- (void)innsmouth;
- (void)dunwich;
@end

@interface MyObject <ProtocolC> @end

// Provide two variants of a base class, one that adopts ProtocolA and
// one that does not.
@interface Lovecraft <ProtocolA> @end
@interface Lovecraft_2 @end

// Provide two variants of a subclass that conform to ProtocolB.  One
// subclasses from a class that conforms to ProtocolA, the other that
// does not.
//
// From those, provide two variants that conformat to ProtocolB_Explicit
// instead.
@interface Shoggoth : Lovecraft <ProtocolB> @end
@interface Shoggoth_2 : Lovecraft_2 <ProtocolB> @end
@interface Shoggoth_Explicit : Lovecraft <ProtocolB_Explicit> @end
@interface Shoggoth_2_Explicit : Lovecraft_2 <ProtocolB_Explicit> @end


@implementation MyObject
- (void)innsmouth {}
- (void)rlyeh {}
- (void)dunwich {}
@end

@implementation Lovecraft
- (void)innsmouth {}
- (void)rlyeh {}
@end

@implementation Shoggoth
- (void)dunwich {}
@end

@implementation Shoggoth_2 // expected-warning {{method 'innsmouth' in protocol 'ProtocolB' not implemented}}\
                           // expected-warning {{method 'rlyeh' in protocol 'ProtocolA' not implemented}}\
                           // expected-warning {{'innsmouth' in protocol 'ProtocolA' not implemented}} 
- (void)dunwich {}
@end

@implementation Shoggoth_Explicit // expected-warning {{method 'innsmouth' in protocol 'ProtocolB_Explicit' not implemented}}
- (void)dunwich {}
@end

@implementation Shoggoth_2_Explicit // expected-warning {{method 'innsmouth' in protocol 'ProtocolB_Explicit' not implemented}}\
                                    // expected-warning {{method 'rlyeh' in protocol 'ProtocolA' not implemented}}\
                                    // expected-warning {{method 'innsmouth' in protocol 'ProtocolA' not implemented}}
- (void)dunwich {}
@end

