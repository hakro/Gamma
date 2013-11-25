#ifndef GAMMA_DFT_H_INC
#define GAMMA_DFT_H_INC

/*	Gamma - Generic processing library
	See COPYRIGHT file for authors and license information */

#include <math.h>
#include "Gamma/mem.h"
#include "Gamma/scl.h"
#include "Gamma/tbl.h"
#include "Gamma/Domain.h"
#include "Gamma/Constants.h"
#include "Gamma/Containers.h"
#include "Gamma/FFT.h"
#include "Gamma/Types.h"


namespace gam{

/// \defgroup DFT

/// Spectral data types
enum SpectralType{
	COMPLEX,	/**< Complex number */
	MAG_PHASE,	/**< Magnitude and phase */
	MAG_FREQ	/**< Magnitude and frequency */
};



/// Sliding window for analysis
    
///\ingroup DFT
///
template <class T=gam::real>
class SlidingWindow{
public:
	SlidingWindow(uint32_t winSize, uint32_t hopSize);
	~SlidingWindow();

	void resize(uint32_t winSize, uint32_t hopSize);
	void sizeHop(uint32_t size);
	void sizeWin(uint32_t size);
	
	uint32_t sizeHop() const;
	uint32_t sizeWin() const;
	
	/// Returns pointer to internal sample window
	
	/// The samples returned cannot be modified directly since they point to an
	/// internal delay line.
	const T * window();
	const T * operator()();
	
	/// Returns true when sample window is ready to be processed
	bool operator()(T input);
	
	/// Returns true when sample window is ready to be processed.
	
	/// Upon returning true, the sample buffer is copied into 'dst'.
	/// 'dst' must have a size of at least sizeWin().
	bool operator()(T * dst, T input);

protected:
	void slide();	// Slides samples in window left by hop num samples
	// After processing the window samples, a call to slide() must be made
	// to prepare for the next window.

protected:
	T * mBuf;
	uint32_t mSizeWin, mSizeHop;
	uint32_t mTapW;	// current index to write to
	uint32_t mHopCnt;	// counts samples for hop
	
private:
	uint32_t hopStart() const;
};



/// Base class for DFTs
    
///\ingroup DFT
///
template <class T=gam::real>
class DFTBase : public DomainObserver{
public:
	DFTBase();
	virtual ~DFTBase();

	/// Get pointer to an auxiliary buffer
	T * aux(uint32_t num);
	
	/// Get pointer to bin data
	Complex<T> * bins() const { return mBins; }
	
	/// Get reference to bin value
	Complex<T>& bin(uint32_t k){ return mBins[k]; }
	
	/// Get read-only reference to bin value
	const Complex<T>& bin(uint32_t k) const { return mBins[k]; }
	
	double binFreq() const;		///< Get width of frequency bins
	uint32_t numBins() const;	///< Get number of frequency bins
	uint32_t sizeDFT() const;	///< Get size of forward transform
	Domain& domainFreq();		///< Get frequency domain

	/// Set number of real-valued auxilliary buffers

	/// Each buffer allocated will maintain a size equal to the number of bins.
	/// Memory is guaranteed to be contiguous. This means if you need a 
	/// complex-valued buffer, you can simply allocate two auxilliary buffers
	/// and use the address of the first one as the complex-valued buffer.
	void numAux(uint32_t num);

	void zero();				///< Zeroes internal frequency bins
	void zeroEnds();			///< Zeroes DC and Nyquist bins
	void zeroAux();				///< Zeroes all auxiliary buffers
	void zeroAux(uint32_t num);	///< Zeroes an auxiliary buffer

	virtual void onDomainChange(double r);

protected:
	uint32_t mSizeDFT, mNumAux;
	union{
		T * mBuf;		// FFT buffer
		Complex<T> * mBins;
	};
	T * mAux;		// aux buffers
	Domain mDomFreq;

	T normForward() const;	// get norm factor for forward transform values
	T * bufPos(){ return mBuf+1; }
	T * bufFrq(){ return mBuf; }
};



/// Discrete Fourier transform

///\ingroup DFT
///
class DFT : public DFTBase<float>{
public:
	/// Constructor

	/// \param[in]	winSize		Number of samples in window
	/// \param[in]	padSize		Number of zeros to append to window
	/// \param[in]	specType	Format of spectrum data
	/// \param[in]	numAux		Number of auxilliary buffers of size numBins() to create
	DFT(
		uint32_t winSize=1024, uint32_t padSize=0,
		SpectralType specType=COMPLEX,
		uint32_t numAux=0
	);

	virtual ~DFT();


	/// Set format of spectrum data
	DFT& spectrumType(SpectralType v);

	/// Set whether to use precise (but slower) for converting to polar
	DFT& precise(bool whether);

	/// Set size parameters of transform
	void resize(uint32_t windowSize, uint32_t padSize);

	float freqRes() const;				///< Get frequency resolution of analysis, \see DFTBase::binFreq
	float overlap() const;				///< Get transform overlap factor
	bool overlapping() const;			///< Whether the transform is overlapping
	uint32_t sizeHop() const;			///< Get size of hop
	uint32_t sizePad() const;			///< Get size of zero-padding
	uint32_t sizeWin() const;			///< Get size of window

	Domain& domainHop();				///< Hop domain

	/// Reads next sample in for a forward transform
	
	/// Returns true when sizeDFT() samples are read in and, subsequently,
	/// the forward DFT is performed.  Returns false otherwise.
	bool operator()(float input);

	/// Returns next sample from inverse transform
	
	/// The inverse transform is performed every sizeWin() samples.
	///
	float operator()();

	/// Performs forward transform on a window of samples
	
	/// 'src' must have at least sizeWin() number of elements.
	///
	void forward(const float * src);	

	/// Performs inverse transform on internal spectrum
	
	///	The resynthesized samples are copied into 'dst'.  The destination
	/// array must have room for at least sizeDFT() number of elements. If 'dst'
	/// equals 0, then the resynthesized samples are not copied, but instead
	/// held in the internal transform buffer.
	virtual void inverse(float * dst);
	
	/// Returns true if next call to inverse() will perform the inverse transform.
	
	/// This method is used for doing inverse-only transforms.
	/// Basically, it tells you when you should set the frequency samples.
	bool inverseOnNext();

	void spctToRect();		// convert spectrum to rectangle format
	void spctToPolar();		// convert spectrum to polar format

	virtual void onDomainChange(double r);
	virtual void print(FILE * fp=stdout, const char * append="\n");
	
protected:
	uint32_t mSizeWin;				// samples in analysis window
	uint32_t mSizeHop;				// samples between forward transforms (= winSize() for DFT)
	SpectralType mSpctFormat;		// format of spectrum
	RFFT<float> mFFT;
	Domain mDomHop;
	
	// Buffers
	float * mPadOA;			// Overlap-add buffer (alloc'ed only if zero-padded)
	float * mBufInv;		// Pointer to inverse sample buffer
	uint32_t mTapW, mTapR;	// DFT i/o read/write taps
	bool mPrecise;
};




/// Short-time Fourier transform

/// The short-time Fourier transform uses a sliding window during analysis
/// to obtain better time resolution between successive spectral frames. The 
/// resolution within each individual spectral frame is still determined by the
/// window size.
///
/// \ingroup DFT
class STFT : public DFT {
public:

	/// \param[in]	winSize		Number of samples to window
	/// \param[in]	hopSize		Number of samples between successive windows
	/// \param[in]	padSize		Number of zeros to append to window
	/// \param[in]	winType		Type of forward transform window
	/// \param[in]	specType	Format of spectrum data
	/// \param[in]	numAux		Number of auxiliary buffers to create
	STFT(uint32_t winSize=1024, uint32_t hopSize=256, uint32_t padSize=0,
		WindowType winType = RECTANGLE,
		SpectralType specType = COMPLEX,
		uint32_t numAux=0
	);
	
	virtual ~STFT();


	using DFT::operator();
	using DFT::sizeHop;


	/// Input next time-domain sample
	
	/// \returns whether a new spectral frame is available
	///
	bool operator()(float input);

	/// Perform forward transform of an array of samples
	void forward(const float * src);
	
	/// Get inverse transform using current spectral frame
	virtual void inverse(float * dst);


	/// Set window and zero-padding size, in samples
	void resize(uint32_t winSize, uint32_t padSize);

	/// Whether to apply a triangular window to inverse transform samples
	STFT& inverseWindowing(bool v){
		mWindowInverse=v; computeInvWinMul(); return *this; }

	/// Whether to rotate input samples by half
	STFT& rotateForward(bool v){ mRotateForward=v; return *this; }

	/// Set hop size, in samples
	STFT& sizeHop(uint32_t size);
	
	/// Set window type
	STFT& windowType(WindowType type);


	double unitsHop();
	
	/// Returns array of current analysis phases (MAG_FREQ format only)
	float * phases();

	/// Returns array of current accumulator phases (MAG_FREQ format only)
	double * accumPhases();

	/// Reset phases (MAG_FREQ format only)

	/// This resets the phases of all the accumulators used in the inverse
	/// transform. It can be used to help eliminate phase smearing artifacts 
	/// from certain transformations, like pitch shifting.
	STFT& resetPhases();
	
	virtual void print(FILE * fp=stdout, const char * append="\n");	

protected:
	void computeInvWinMul();	// compute inverse normalization factor (due to overlap-add)
	
	SlidingWindow<float> mSlide;
	float * mFwdWin;			// forward transform window
	float * mPhases;			// copy of current phases (mag-freq mode)
	double * mAccums;			// phase accumulators (mag-freq mode)
	WindowType mWinType;		// type of analysis window used
	float mFwdWinMul, mInvWinMul;
	bool mWindowInverse;		// whether to window inverse samples
	bool mRotateForward;
};




/// Sliding discrete Fourier transform

/// This transform computes the DFT with a fixed hop size of 1 sample and
/// within a specified frequency interval. The computational complexity per
/// sample is O(M), where M is the size, in samples, of the frequency interval.
///
/// \ingroup DFT
template <class T>
class SlidingDFT : public DFTBase<T> {
public:

	/// \param[in] sizeDFT	transform size, in samples
	/// \param[in] binLo	lower closed endpoint of frequency interval
	/// \param[in] binHi	upper open endpoint of frequency interval
	SlidingDFT(uint32_t sizeDFT, uint32_t binLo, uint32_t binHi);
	
	/// Input next sample and perform forward transform
	void forward(T input);
	
	/// Set endpoints of frequency interval
	SlidingDFT& interval(uint32_t binLo, uint32_t binHi);

	/// Resize transform
	void resize(uint32_t sizeDFT, uint32_t binLo, uint32_t binHi);
		
protected:
	uint32_t mBinLo, mBinHi;
	DelayN<T> mDelay;
	//T * mBufI;				// alias to imaginaries
	Complex<T> mF1;
	Complex<T> mFL;
	//double mC0, mS0;		// phasor rotators
	//double mCL, mSL;		// low bin phasor
	T mNorm;				// fwd transform normalization
};




// Implementation_______________________________________________________________
template<class T>
SlidingWindow<T>::SlidingWindow(uint32_t winSize, uint32_t hopSize)
:	mBuf(0), mSizeWin(0), mSizeHop(0), mHopCnt(0)
{
	resize(winSize, hopSize);
	mem::deepZero(mBuf, sizeWin());
}

template<class T>
SlidingWindow<T>::~SlidingWindow(){
	if(mBuf){ free(mBuf); mBuf = 0; }
}

template<class T>
void SlidingWindow<T>::resize(uint32_t winSize, uint32_t hopSize){
	sizeWin(winSize);
	sizeHop(hopSize);
	//mTapW = hopStart();	// for single-buffer slide mode
	mTapW = 0;				// for single-buffer rotate mode
}

template<class T>
void SlidingWindow<T>::sizeWin(uint32_t size){
	if(mem::resize(mBuf, sizeWin(), size)) mSizeWin = size;
}

template<class T>
void SlidingWindow<T>::sizeHop(uint32_t size){
	mSizeHop = scl::clip(size, sizeWin(), (uint32_t)1);
}

template<class T>
inline uint32_t SlidingWindow<T>::sizeHop() const { return mSizeHop; }
    
template<class T>
inline uint32_t SlidingWindow<T>::sizeWin() const { return mSizeWin; }
    
template<class T>
inline const T * SlidingWindow<T>::window(){ return mBuf; }
    
template<class T>
inline const T * SlidingWindow<T>::operator()(){ return mBuf; }

template<class T>
inline bool SlidingWindow<T>::operator()(T input){

// faster, but requires explicit call to slide() by user which is error prone
//	mBuf[mTapW] = input;
//	
//	if(++mTapW == sizeWin()){
//		mTapW = hopStart();
//		return true;
//	}
//	return false;
	
	mBuf[mTapW] = input;
	
	if(++mTapW >= sizeHop()){
		mTapW = 0;
		mem::rotateLeft(sizeHop(), mBuf, sizeWin());
		return true;
	}
	return false;
}

template<class T>
inline bool SlidingWindow<T>::operator()(T * output, T input){
	mBuf[mTapW] = input;
	if(++mTapW == sizeWin()) mTapW = 0;

	if(++mHopCnt == sizeHop()){
		mem::copyAllFromRing(mBuf, sizeWin(), mTapW, output);
		mHopCnt = 0;
		return true;
	}
	return false;
}

template<class T>
void SlidingWindow<T>::slide(){
	mem::deepMove(mBuf, mBuf + sizeHop(), hopStart());
}

template<class T>
inline uint32_t SlidingWindow<T>::hopStart() const { return sizeWin() - sizeHop(); }




template<class T>
DFTBase<T>::DFTBase()
:	mSizeDFT(0), mNumAux(0), mBuf(0), mAux(0)
{
	refreshDomain();
}

template<class T>
DFTBase<T>::~DFTBase(){ //printf("~DFTBase\n");
	mem::free(mBuf);
	mem::free(mAux);
}

template<class T>
inline T * DFTBase<T>::aux(uint32_t num){ return mAux + numBins() * num; }
    
template<class T>
inline double DFTBase<T>::binFreq() const { return spu() / sizeDFT(); }
    
template<class T>
inline uint32_t	DFTBase<T>::numBins() const { return (sizeDFT() + 2)>>1; }
    
template<class T>
inline uint32_t	DFTBase<T>::sizeDFT() const { return mSizeDFT; }
    
template<class T>
inline Domain& DFTBase<T>::domainFreq(){ return mDomFreq; }
    
template<class T>
inline T DFTBase<T>::normForward() const { return T(2) / T(sizeDFT()); }

template<class T>
void DFTBase<T>::numAux(uint32_t num){
	if(mem::resize(mAux, mNumAux * numBins(), num * numBins())){
		mNumAux = num;
		zeroAux();
	}
}

template<class T>
void DFTBase<T>::zero(){ mem::deepZero(mBuf, sizeDFT() + 2); }

template<class T>
void DFTBase<T>::zeroEnds(){
	bins()[0](0,0);
	bins()[numBins()-1](0,0);
}

template<class T>
void DFTBase<T>::zeroAux(){ mem::deepZero(mAux, mNumAux * numBins()); }

template<class T>
void DFTBase<T>::zeroAux(uint32_t num){ mem::deepZero(aux(num), numBins()); }

template<class T>
void DFTBase<T>::onDomainChange(double r){
	domainFreq().ups(binFreq());
}




inline DFT& DFT::spectrumType(SpectralType v){ mSpctFormat=v; return *this; }
inline DFT& DFT::precise(bool w){ mPrecise=w; return *this; }

inline float DFT::freqRes() const { return spu() / sizeWin(); }
inline float DFT::overlap() const { return float(sizeWin()) / sizeHop(); }
inline bool DFT::overlapping() const { return sizeHop() < sizeWin(); }
inline uint32_t DFT::sizeHop() const { return mSizeHop; }
inline uint32_t DFT::sizePad() const { return mSizeDFT - mSizeWin; }
inline uint32_t DFT::sizeWin() const { return mSizeWin; }
inline Domain& DFT::domainHop(){ return mDomHop; }

inline bool DFT::operator()(float input){
	bufPos()[mTapW] = input;

	if(++mTapW >= sizeHop()){
		forward(bufPos());
		mTapW = 0;
		return true;
	}
	return false;
}

inline float DFT::operator()(){
	if(++mTapR >= sizeHop()){
		inverse(0);	// this is a virtual method
		mTapR = 0;
	}
	return mBufInv[mTapR];
}

inline bool DFT::inverseOnNext(){ return mTapR == (sizeHop() - 1); }




inline bool STFT::operator()(float input){
	if(mSlide(bufPos(), input)){
		forward(bufPos());
		return true;
	}
	return false;
}

inline double STFT::unitsHop(){ return DFT::sizeHop() * ups(); }
inline float * STFT::phases(){ return mPhases; }
inline double * STFT::accumPhases(){ return mAccums; }




template<class T>
SlidingDFT<T>::SlidingDFT(uint32_t sizeDFT, uint32_t binLo, uint32_t binHi)
	: DFTBase<T>(), mBinLo(0), mBinHi(0), mDelay(0)
{
	resize(sizeDFT, binLo, binHi);
}

template<class T>
void SlidingDFT<T>::resize(uint32_t sizeDFT, uint32_t binLo, uint32_t binHi){
	// may be able to keep these smaller?
	mem::resize(this->mBuf, this->mSizeDFT + 2, sizeDFT + 2);
	mem::deepZero(this->mBuf, sizeDFT + 2);

	mDelay.resize(sizeDFT);
	mDelay.assign(T(0));

	this->mSizeDFT = sizeDFT;

	interval(binLo, binHi);

	//this->onSyncChange();
}

template<class T>
SlidingDFT<T>& SlidingDFT<T>::interval(uint32_t binLo, uint32_t binHi){
	mBinLo = binLo;
	mBinHi = binHi;
	
	double theta = M_2PI / this->sizeDFT();

	mF1.fromPhase(theta);
	mFL.fromPhase(theta*mBinLo);
	mNorm = T(2) / T(this->sizeDFT());
	return *this;
}

//
template<class T>
inline void SlidingDFT<T>::forward(T input){
	T dif = (input - mDelay(input)) * mNorm;	// ffd comb zeroes
												// difference between temporal 'frames'
	Complex<T> c = mFL;							// phasor at low bin

	// apply complex resonators:
	// multiply freq samples by 1st harmonic (shift time signal)
	// add time sample to all bins (set time sample at n=0)
	
	for(uint32_t k=mBinLo; k<mBinHi; ++k){
		Complex<T>& b = this->bins(k);
		b = b*c + dif;
		c *= mF1;
	}
}

//template<class T>
//inline T SlidingDFT<T>::inverse(){}

/*
template<class T>
class Counter{
public:

	bool operator()(){
		if(++mVal == mMax){
			onWrap();
			mVal = (T)0;
			return true;
		}
		return false;
	}

	virtual void onWrap();

protected:
	T mVal, mMax;
};
*/

} // gam::
#endif
