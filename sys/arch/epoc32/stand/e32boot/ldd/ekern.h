/*******************************************************************
 *
 *	File:		EKern.h
 *
 *	Author:		Peter van Sebille (peter@yipton.net)
 *
 *	(c) Copyright 2001, Peter van Sebille
 *	All Rights Reserved
 *
 *******************************************************************/


#ifndef __EKERN_H
#define __EKERN_H

//#ifdef __WINS__
#ifdef USE_ASM_INT3
#define INT3	_asm int 3
#else
#define INT3
#endif

class DLogicalChannel : public CObject
{
protected: IMPORT_C DLogicalChannel(class DLogicalDevice *);
protected: IMPORT_C virtual ~DLogicalChannel(void);
protected: IMPORT_C virtual void Close(void);
protected: IMPORT_C void SetBehaviour(unsigned int);
protected: IMPORT_C void Complete(int);
protected: IMPORT_C void Complete(int,int);
protected: IMPORT_C void CompleteAll(int);
	// probably pure virtuals

virtual void DoCancel(TInt aReqNo) = 0;
virtual void DoRequest(TInt aReqNo,TAny *a1,TAny *a2) = 0;
protected: /*IMPORT_C*/ virtual void DoCreateL(int,class CBase *,class TDesC8 const *,class TVersion const &){};
protected: IMPORT_C virtual int DoControl(int,void *,void *);

	// hmm, loading the driver crashed on the Series 5, whereas it worked ok on the S5mx + S7
	// added a few bogus virtuals seems to do the trick 
protected: IMPORT_C virtual void foo1(){};
protected: IMPORT_C virtual void foo2(){};
protected: IMPORT_C virtual void foo3(){};
protected: IMPORT_C virtual void foo4(){};
protected: IMPORT_C virtual void foo5(){};
protected: IMPORT_C virtual void foo6(){};

	TInt	NoOfKernChunks();

	TUint	iUnknown[1024];
};



/*
 * DLogicalDevice is probably derived from CObject as the original Arlo
 * calls SetName from Install
 */
class DLogicalDevice : public CObject
{
public: IMPORT_C virtual int Remove(void);
public: IMPORT_C virtual int QueryVersionSupported(class TVersion const &)const;
public: IMPORT_C virtual int IsAvailable(int,class TDesC8 const *,class TDesC8 const *)const;

public:		// these seem to be pure virtuals

	virtual TInt Install() = 0;
	virtual void GetCaps(TDes8 &aDes) const = 0;
	virtual DLogicalChannel *CreateL() = 0;

protected: IMPORT_C DLogicalDevice(void);
protected: IMPORT_C virtual ~DLogicalDevice(void);
	TInt		iUnknownData[1024];
};








#endif /* __EKERN_H */
