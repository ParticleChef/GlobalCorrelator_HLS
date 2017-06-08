#include <cstdio>
#include "simple_pflow.h"

#define NTEST 500

bool pf_equals(const PFChargedObj &out_ref, const PFChargedObj &out, const char *what, int idx) {
	bool ret;
	if (out_ref.hwPt == 0) {
		ret = (out.hwPt == 0);
	} else {
		ret = (out_ref.hwPt == out.hwPt && out_ref.hwEta == out.hwEta && out_ref.hwPhi == out.hwPhi && out_ref.hwId  == out.hwId);
	}
	if  (!ret) {
		printf("Mismatch at %s[%3d], hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d \n", what, idx,
				int(out_ref.hwPt), int(out.hwPt),
				int(out_ref.hwEta), int(out.hwEta),
				int(out_ref.hwPhi), int(out.hwPhi),
				int(out_ref.hwId), int(out.hwId));
	}
	return ret;
}
bool pf_equals(const PFNeutralObj &out_ref, const PFNeutralObj &out, const char *what, int idx) {
	bool ret;
	if (out_ref.hwPt == 0) {
		ret = (out.hwPt == 0);
	} else {
		ret = (out_ref.hwPt == out.hwPt && out_ref.hwEta == out.hwEta && out_ref.hwPhi == out.hwPhi && out_ref.hwId  == out.hwId);
	}
	if  (!ret) {
		printf("Mismatch at %s[%3d], hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d \n", what, idx,
				int(out_ref.hwPt), int(out.hwPt),
				int(out_ref.hwEta), int(out.hwEta),
				int(out_ref.hwPhi), int(out.hwPhi),
				int(out_ref.hwId), int(out.hwId));
	}
	return ret;
}
int main() {

	srand(37); // 37 is a good random number
	
	CaloObj calo[NCALO]; TkObj track[NTRACK];
    PFChargedObj outch[NTRACK], outch_ref[NTRACK];
    PFNeutralObj outne[NCALO], outne_ref[NCALO];

	for (int test = 1; test <= NTEST; ++test) {
		for (int i = 0; i < NTRACK; ++i) {
			track[i].hwPt = 0; track[i].hwPtErr = 0; track[i].hwEta = 0; track[i].hwPhi = 0;
		}
		for (int i = 0; i < NCALO; ++i) {
			calo[i].hwPt = 0; calo[i].hwEta = 0; calo[i].hwPhi = 0;
		}
		int ncharged = (rand() % NTRACK/2) + NTRACK/2;
		int nneutral = (rand() % ((3*NCALO)/4));
		for (int i = 1; i < nneutral && i < NCALO; i += 2) {
			float pt = (rand()/float(RAND_MAX))*80+1, eta = (rand()/float(RAND_MAX))*2.0-1.0, phi = (rand()/float(RAND_MAX))*2.0-1.0;
			calo[i].hwPt  = pt * PT_SCALE;
			calo[i].hwEta = eta * ETAPHI_SCALE;
			calo[i].hwPhi = phi * ETAPHI_SCALE;
		}
		for (int i = 0; i < ncharged && i < NTRACK; ++i) {
			float pt = (rand()/float(RAND_MAX))*50+2, eta = (rand()/float(RAND_MAX))*2.0-1.0, phi = (rand()/float(RAND_MAX))*2.0-1.0;
			track[i].hwPt    = pt * PT_SCALE;
			track[i].hwPtErr = (0.2*pt+4) * PT_SCALE; 
			track[i].hwEta = eta * ETAPHI_SCALE;
			track[i].hwPhi = phi * ETAPHI_SCALE;
			int icalo = rand() % NCALO;
			if (i % 3 == 1 || icalo >= NCALO) continue;
			float dpt_calo = ((rand()/float(RAND_MAX))*3-1.5) * (0.2*pt+4);
			float deta_calo = ((rand()/float(RAND_MAX))*0.3-0.15), dphi_calo = ((rand()/float(RAND_MAX))*0.3-0.15);
			if (pt + dpt_calo > 0) {
				calo[icalo].hwPt  += (pt + dpt_calo) * PT_SCALE;
				calo[icalo].hwEta = (eta + deta_calo) * ETAPHI_SCALE;
				calo[icalo].hwPhi = (phi + dphi_calo) * ETAPHI_SCALE;
			}
		}

		//simple_pflow_iterative_ref(calo, track, out_ref);
		//simple_pflow_iterative_hwopt(calo, track, out);
		simple_pflow_parallel_ref(calo, track, outch_ref, outne_ref);
		simple_pflow_parallel_hwopt(calo, track, outch, outne);

// ---------------- COMPARE WITH EXPECTED ----------------

		int errors = 0; int ntot = 0, nch = 0, nneu = 0;
		for (int i = 0; i < NTRACK; ++i) {
			if (!pf_equals(outch_ref[i], outch[i], "PF Charged", i)) errors++;
			if (outch_ref[i].hwPt > 0) { ntot++; nch++; }
		}
		for (int i = 0; i < NCALO; ++i) {
			if (!pf_equals(outne_ref[i], outne[i], "PF Neutral", i)) errors++;
			if (outne_ref[i].hwPt > 0) { ntot++; nneu++; }
		}
		if (errors != 0) {
			printf("Error in computing test %d (%d)\n", test, errors);
			for (int i = 0; i < NCALO; ++i) {
				printf("calo  %3d, hwPt % 7d   hwPtErr % 7d    hwEta %+7d   hwPhi %+7d\n", i, int(calo[i].hwPt), 0, int(calo[i].hwEta), int(calo[i].hwPhi));
			}
			for (int i = 0; i < NTRACK; ++i) {
				printf("track %3d, hwPt % 7d   hwPtErr % 7d    hwEta %+7d   hwPhi %+7d\n", i, int(track[i].hwPt), int(track[i].hwPtErr), int(track[i].hwEta), int(track[i].hwPhi));
			}
			for (int i = 0; i < NTRACK; ++i) {
				printf("charged pf %3d, hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d \n", i,
					int(outch_ref[i].hwPt), int(outch[i].hwPt), int(outch_ref[i].hwEta), int(outch[i].hwEta),
					int(outch_ref[i].hwId), int(outch[i].hwPhi), int(outch_ref[i].hwId), int(outch[i].hwId));
			}
			for (int i = 0; i < NCALO; ++i) {
				printf("neutral pf %3d, hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d \n", i,
					int(outne_ref[i].hwPt), int(outne[i].hwPt), int(outne_ref[i].hwEta), int(outne[i].hwEta),
					int(outne_ref[i].hwId), int(outne[i].hwPhi), int(outne_ref[i].hwId), int(outne[i].hwId));
			}
			return 1;
		} else {
			printf("Passed test %d (%d, %d, %d)\n", test, ntot, nch, nneu);
		}
	}	
	return 0;
}
