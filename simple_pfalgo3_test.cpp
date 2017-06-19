#include <cstdio>
#include "src/simple_pfalgo3.h"
#include "random_inputs.h"
#include "DiscretePFInputs_IO.h"

#define NTEST 500

bool pf_equals(const PFChargedObj &out_ref, const PFChargedObj &out, const char *what, int idx) {
    bool ret;
    if (out_ref.hwPt == 0) {
        ret = (out.hwPt == 0);
    } else {
        ret = (out_ref.hwPt == out.hwPt && out_ref.hwEta == out.hwEta && out_ref.hwPhi == out.hwPhi && out_ref.hwId  == out.hwId && out_ref.hwZ0  == out.hwZ0);
    }
    if  (!ret) {
        printf("Mismatch at %s[%3d], hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d      hwZ0 %+7d %+7d   \n", what, idx,
                int(out_ref.hwPt), int(out.hwPt),
                int(out_ref.hwEta), int(out.hwEta),
                int(out_ref.hwPhi), int(out.hwPhi),
                int(out_ref.hwId), int(out.hwId),
                int(out_ref.hwZ0), int(out.hwZ0));
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

    //RandomPFInputs inputs(37); // 37 is a good random number
    DiscretePFInputs inputs("regions_TTbar_PU140.dump");
    
    HadCaloObj calo[NCALO]; EmCaloObj emcalo[NEMCALO]; TkObj track[NTRACK]; z0_t hwZPV;
    PFChargedObj outch[NTRACK], outch_ref[NTRACK];
    PFNeutralObj outpho[NPHOTON], outpho_ref[NPHOTON];
    PFNeutralObj outne[NSELCALO], outne_ref[NSELCALO];

    for (int test = 1; test <= NTEST; ++test) {
        for (int i = 0; i < NTRACK; ++i) {
            track[i].hwPt = 0; track[i].hwPtErr = 0; track[i].hwEta = 0; track[i].hwPhi = 0; track[i].hwZ0 = 0;
        }
        for (int i = 0; i < NCALO; ++i) {
            calo[i].hwPt = 0; calo[i].hwEmPt = 0; calo[i].hwEta = 0; calo[i].hwPhi = 0;
        }
        for (int i = 0; i < NEMCALO; ++i) {
            emcalo[i].hwPt = 0;emcalo[i].hwPtErr = 0;  emcalo[i].hwEta = 0; emcalo[i].hwPhi = 0;
        }


        if (!inputs.nextRegion(calo, emcalo, track, hwZPV)) break;

        //pfalgo3_ref(calo, track, outch_ref, outne_ref);
        //tk2calo_algo(calo, track, outch, outne);
        bool isEle[NTRACK], isEle_ref[NTRACK];
        tk2em_step1_ref(emcalo, track, isEle_ref, outpho_ref);
        tk2em_step1(emcalo, track, isEle, outpho);
        int errors = 0; int ntot = 0, npho = 0, nch = 0, nneu = 0;
#if 1
        for (int i = 0; i < NTRACK; ++i) {
            if (track[i].hwPt > 0 && isEle[i] != isEle_ref[i]) { 
                printf("Electron mismatch for track %2d (hw %d, ref %d)\n", i, int(isEle[i]), int(isEle_ref[i]));
                errors++;
            }
        }
        for (int i = 0; i < NPHOTON; ++i) {
            if (!pf_equals(outpho_ref[i], outpho[i], "Photon", i)) errors++;
            if (outpho_ref[i].hwPt > 0) { ntot++; npho++; }
        }

#else
        for (int i = 0; i < NTRACK; ++i) {
            if (!pf_equals(outch_ref[i], outch[i], "PF Charged", i)) errors++;
            if (outch_ref[i].hwPt > 0) { ntot++; nch++; }
        }
        for (int i = 0; i < NSELCALO; ++i) {
            if (!pf_equals(outne_ref[i], outne[i], "PF Neutral", i)) errors++;
            if (outne_ref[i].hwPt > 0) { ntot++; nneu++; }
        }
#endif
        if (errors != 0) {
            printf("Error in computing test %d (%d)\n", test, errors);
            for (int i = 0; i < NCALO; ++i) {
                printf("calo  %3d, hwPt % 7d   hwEmPt  % 7d    hwEta %+7d   hwPhi %+7d\n", i, int(calo[i].hwPt), int(calo[i].hwEmPt), int(calo[i].hwEta), int(calo[i].hwPhi));
            }
            for (int i = 0; i < NEMCALO; ++i) {
                printf("em    %3d, hwPt % 7d   hwPtErr % 7d    hwEta %+7d   hwPhi %+7d\n", i, int(emcalo[i].hwPt), int(emcalo[i].hwPtErr), int(emcalo[i].hwEta), int(emcalo[i].hwPhi));
            }
            for (int i = 0; i < NTRACK; ++i) {
                printf("track %3d, hwPt % 7d   hwPtErr % 7d    hwEta %+7d   hwPhi %+7d     hwZ0 %+7d\n", i, int(track[i].hwPt), int(track[i].hwPtErr), int(track[i].hwEta), int(track[i].hwPhi), int(track[i].hwZ0));
            }
            for (int i = 0; i < NTRACK; ++i) {
                printf("charged pf %3d, hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d      hwZ0 %+7d %+7d\n", i,
                    int(outch_ref[i].hwPt), int(outch[i].hwPt), int(outch_ref[i].hwEta), int(outch[i].hwEta),
                    int(outch_ref[i].hwPhi), int(outch[i].hwPhi), int(outch_ref[i].hwId), int(outch[i].hwId),
                    int(outch_ref[i].hwZ0), int(outch[i].hwZ0));
            }
            for (int i = 0; i < NSELCALO; ++i) {
                printf("neutral pf %3d, hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d\n", i,
                    int(outne_ref[i].hwPt), int(outne[i].hwPt), int(outne_ref[i].hwEta), int(outne[i].hwEta),
                    int(outne_ref[i].hwPhi), int(outne[i].hwPhi), int(outne_ref[i].hwId), int(outne[i].hwId));
            }
            for (int i = 0; i < NPHOTON; ++i) {
                printf("photon  pf %3d, hwPt % 7d % 7d   hwEta %+7d %+7d   hwPhi %+7d %+7d   hwId %1d %1d\n", i,
                    int(outpho_ref[i].hwPt), int(outpho[i].hwPt), int(outpho_ref[i].hwEta), int(outpho[i].hwEta),
                    int(outpho_ref[i].hwPhi), int(outpho[i].hwPhi), int(outpho_ref[i].hwId), int(outpho[i].hwId));
            }

            return 1;
        } else {
            printf("Passed test %d (%d, %d, %d, %d)\n", test, ntot, nch, npho, nneu);
        }

    }
    return 0;
}
