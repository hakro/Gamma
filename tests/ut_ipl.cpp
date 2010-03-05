#include "ut.h"

int ut_ipl(){

	{
		const int N=4;
		Array<double> a(N);
		for(int i=0; i<N; ++i) a[i]=i;
		
		assert(ipl::linear(0.5, a[0], a[1]) == 0.5);
		assert(ipl::Linear()(acc::Wrap(), a,   0, 0.5, N-1,0) == 0.5);
		assert(ipl::Linear()(acc::Wrap(), a, N-1, 0.5, N-1,0) == (N-1)/2.);
		assert(ipl::Linear()(acc::Clip(), a,   0, 0.5, N-1,0) == 0.5);
		assert(ipl::Linear()(acc::Clip(), a, N-1, 0.5, N-1,0) == (N-1));
	}

	
	return 0;	
}
