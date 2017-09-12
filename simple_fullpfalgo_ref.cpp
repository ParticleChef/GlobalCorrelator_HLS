#include "src/data.h"
#include "src/simple_fullpfalgo.h"
#include <cmath>
#include <algorithm>

template <typename T> int sqr(const T & t) { return t*t; }

template<int NCAL, int DR2MAX, bool doPtMin, typename CO_t>
int best_match_ref(CO_t calo[NCAL], const TkObj & track) {
    pt_t caloPtMin = track.hwPt - 2*(track.hwPtErr);
    if (caloPtMin < 0) caloPtMin = 0;
    int  drmin = DR2MAX, ibest = -1;
    for (int ic = 0; ic < NCAL; ++ic) {
            if (doPtMin && calo[ic].hwPt <= caloPtMin) continue;
            int dr = dr2_int(track.hwEta, track.hwPhi, calo[ic].hwEta, calo[ic].hwPhi);
            if (dr < drmin) { drmin = dr; ibest = ic; }
    }
    return ibest;
}
template<int NCAL, int DR2MAX>
int best_match_ref(HadCaloObj calo[NCAL], const EmCaloObj & em) {
    pt_t emPtMin = em.hwPt >> 1;
    int  drmin = DR2MAX, ibest = -1;
    for (int ic = 0; ic < NCAL; ++ic) {
            if (calo[ic].hwEmPt <= emPtMin) continue;
            int dr = dr2_int(em.hwEta, em.hwPhi, calo[ic].hwEta, calo[ic].hwPhi);
            if (dr < drmin) { drmin = dr; ibest = ic; }
    }
    return ibest;
}

template<int NCAL, int DR2MAX, typename CO_t>
int best_match_with_pt_ref(CO_t calo[NCAL], const TkObj & track) {
    pt_t caloPtMin = track.hwPt - 2*(track.hwPtErr);
    if (caloPtMin < 0) caloPtMin = 0;
    int dptscale = (DR2MAX<<8)/std::max<int>(1,sqr(track.hwPtErr));
    int drmin = 0, ibest = -1;
    for (int ic = 0; ic < NCAL; ++ic) {
            if (calo[ic].hwPt <= caloPtMin) continue;
            int dr = dr2_int(track.hwEta, track.hwPhi, calo[ic].hwEta, calo[ic].hwPhi);
            if (dr >= DR2MAX) continue;
            dr += (( sqr(std::max<int>(track.hwPt-calo[ic].hwPt,0))*dptscale ) >> 8);
            //printf("REF DQ(track %+7d %+7d  calo %3d) = %12d\n", int(track.hwEta), int(track.hwPhi), ic, dr);
            if (ibest == -1 || dr < drmin) { drmin = dr; ibest = ic; }
    }
    return ibest;
}


template<int NCAL, int DR2MAX, bool doPtMin, typename CO_t>
void link_ref(CO_t calo[NCAL], TkObj track[NTRACK], ap_uint<NCAL> calo_track_link_bit[NTRACK]) {
    for (int it = 0; it < NTRACK; ++it) {
        int ibest = best_match_ref<NCALO,DR2MAX,doPtMin,CO_t>(calo, track[it]);
        calo_track_link_bit[it] = 0;
        if (ibest != -1) calo_track_link_bit[it][ibest] = 1;
    }
}

template<typename T, int NIn, int NOut>
void ptsort_ref(T in[NIn], T out[NOut]) {
    for (int iout = 0; iout < NOut; ++iout) {
        out[iout].hwPt = 0;
    }
    int nout = 0;
    for (int it = 0; it < NIn; ++it) {
        for (int iout = 0; iout < NOut; ++iout) {
            if (in[it].hwPt >= out[iout].hwPt) {
                for (int i2 = NOut-1; i2 > iout; --i2) {
                    out[i2] = out[i2-1];
                }
                out[iout] = in[it];
                break;
            }
        }
    }
}


void pfalgo3_calo_ref(HadCaloObj calo[NCALO], TkObj track[NTRACK], PFChargedObj outch[NTRACK], PFNeutralObj outne[NCALO]) {
    // constants
    const pt_t     TKPT_MAX = PFALGO3_TK_MAXINVPT; // 20 * PT_SCALE;
    const int      DR2MAX   = PFALGO3_DR2MAX_TK_CALO;

    // initialize sum track pt
    pt_t calo_sumtk[NCALO], calo_subpt[NCALO];
    int  calo_sumtkErr2[NCALO];
    for (int ic = 0; ic < NCALO; ++ic) { calo_sumtk[ic] = 0;  calo_sumtkErr2[ic] = 0;}

    // initialize good track bit
    bool track_good[NTRACK];
    for (int it = 0; it < NTRACK; ++it) { track_good[it] = (track[it].hwPt < TKPT_MAX); }

    // initialize output
    for (int ipf = 0; ipf < NTRACK; ++ipf) { outch[ipf].hwPt = 0; }
    for (int ipf = 0; ipf < NSELCALO; ++ipf) { outne[ipf].hwPt = 0; }

    // for each track, find the closest calo
    for (int it = 0; it < NTRACK; ++it) {
        if (track[it].hwPt > 0) {
            //int  ibest = best_match_ref<NCALO,DR2MAX,true,HadCaloObj>(calo, track[it]);
            int  ibest = best_match_with_pt_ref<NCALO,DR2MAX,HadCaloObj>(calo, track[it]);
            if (ibest != -1) {
                track_good[it] = 1;
                calo_sumtk[ibest]    += track[it].hwPt;
                calo_sumtkErr2[ibest] += sqr(track[it].hwPtErr);
            }
        }
    }

    for (int ic = 0; ic < NCALO; ++ic) {
        if (calo_sumtk[ic] > 0) {
            pt_t ptdiff = calo[ic].hwPt - calo_sumtk[ic];
            if (ptdiff > 0 && ptdiff*ptdiff > 4*calo_sumtkErr2[ic]) {
                calo_subpt[ic] = ptdiff;
            } else {
                calo_subpt[ic] = 0;
            }
        } else {
            calo_subpt[ic] = calo[ic].hwPt;
        }
    }

    // copy out charged hadrons
    for (int it = 0; it < NTRACK; ++it) {
        if (track_good[it]) {
            outch[it].hwPt = track[it].hwPt;
            outch[it].hwEta = track[it].hwEta;
            outch[it].hwPhi = track[it].hwPhi;
            outch[it].hwZ0 = track[it].hwZ0;
            outch[it].hwId  = PID_Charged;
        }
    }

    // copy out neutral hadrons
    PFNeutralObj outne_all[NCALO];
    for (int ipf = 0; ipf < NCALO; ++ipf) { outne_all[ipf].hwPt = 0; }
    for (int ic = 0; ic < NCALO; ++ic) {
        if (calo_subpt[ic] > 0) {
            outne_all[ic].hwPt  = calo_subpt[ic];
            outne_all[ic].hwEta = calo[ic].hwEta;
            outne_all[ic].hwPhi = calo[ic].hwPhi;
            outne_all[ic].hwId  = PID_Neutral;
        }
    }

    ptsort_ref<PFNeutralObj,NCALO,NSELCALO>(outne_all, outne);
}

void pfalgo3_em_ref(EmCaloObj emcalo[NEMCALO], HadCaloObj hadcalo[NCALO], TkObj track[NTRACK], bool isEle[NTRACK], bool isMu[NTRACK], PFNeutralObj outpho[NPHOTON], HadCaloObj hadcalo_out[NCALO]) {
    // constants
    const int DR2MAX_TE = PFALGO3_DR2MAX_TK_EM;
    const int DR2MAX_EH = PFALGO3_DR2MAX_EM_CALO;

    // initialize sum track pt
    pt_t calo_sumtk[NEMCALO];
    for (int ic = 0; ic < NEMCALO; ++ic) {  calo_sumtk[ic] = 0; }
    int tk2em[NTRACK]; 
    bool isEM[NEMCALO];
    // for each track, find the closest calo
    for (int it = 0; it < NTRACK; ++it) {
        if (track[it].hwPt > 0 && !isMu[it]) {
            tk2em[it] = best_match_ref<NEMCALO,DR2MAX_TE,false,EmCaloObj>(emcalo, track[it]);
            // printf("C++: tk not 0 index = %i and matched calo index = %i \n", it, int(tk2em[it]) );
            if (tk2em[it] != -1) {
                calo_sumtk[tk2em[it]] += track[it].hwPt;
            }
        } else {
        	tk2em[it] = -1;
        }
        // printf("C++: tk index = %i and match = %i and sumtk = %i \n", it, int(tk2em[it]), int(calo_sumtk[tk2em[it]]));        
    }

    // for (int ic = 0; ic < NEMCALO; ++ic) {  printf("C++: calo_sumtk[NEMCALO] = %i \n", int(calo_sumtk[ic])); }

    for (int ic = 0; ic < NEMCALO; ++ic) {
        pt_t photonPt;
        if (calo_sumtk[ic] > 0) {
            pt_t ptdiff = emcalo[ic].hwPt - calo_sumtk[ic];
            if (ptdiff*ptdiff <= 4*sqr(emcalo[ic].hwPtErr)) {
                // electron
                photonPt = 0; 
                isEM[ic] = true;
            } else if (ptdiff > 0) {
                // electron + photon
                photonPt = ptdiff; 
                isEM[ic] = true;
            } else {
                // pion
                photonPt = 0;
                isEM[ic] = false;
            }
        } else {
            // photon
            isEM[ic] = true;
            photonPt = emcalo[ic].hwPt;
        }
        outpho[ic].hwPt  = photonPt;
        outpho[ic].hwEta = photonPt ? emcalo[ic].hwEta : etaphi_t(0);
        outpho[ic].hwPhi = photonPt ? emcalo[ic].hwPhi : etaphi_t(0);
        outpho[ic].hwId  = photonPt ? PID_Photon : particleid_t(0);

        // printf("C++: emcalo index = %i and pt = %i and calo_sumtk = %i \n", ic, int(photonPt),int(calo_sumtk[ic]));
    }

    for (int it = 0; it < NTRACK; ++it) {
        isEle[it] = (tk2em[it] != -1) && isEM[tk2em[it]];
    }

    int em2calo[NEMCALO];
    for (int ic = 0; ic < NEMCALO; ++ic) {
        em2calo[ic] = best_match_ref<NCALO,DR2MAX_EH>(hadcalo, emcalo[ic]);
    }
    
    for (int ih = 0; ih < NCALO; ++ih) {
        hadcalo_out[ih] = hadcalo[ih];
        pt_t sub = 0;
        for (int ic = 0; ic < NEMCALO; ++ic) {
            if (isEM[ic] && (em2calo[ic] == ih)) {
                sub += emcalo[ic].hwPt;
            }
        }
        pt_t emdiff  = hadcalo[ih].hwEmPt - sub;
        pt_t alldiff = hadcalo[ih].hwPt - sub;
        if (alldiff < ( hadcalo[ih].hwPt >>  4 ) ) {
            hadcalo_out[ih].hwPt = 0;   // kill
            hadcalo_out[ih].hwEmPt = 0; // kill
        } else if (hadcalo[ih].hwIsEM && emdiff < ( hadcalo[ih].hwEmPt >> 3 ) ) {
            hadcalo_out[ih].hwPt = 0;   // kill
            hadcalo_out[ih].hwEmPt = 0; // kill
        } else {
            hadcalo_out[ih].hwPt   = alldiff;   
            hadcalo_out[ih].hwEmPt = (emdiff > 0 ? emdiff : pt_t(0)); 
        }
    }
}

void pfalgo3_full_ref(EmCaloObj emcalo[NEMCALO], HadCaloObj hadcalo[NCALO], TkObj track[NTRACK], MuObj mu[NMU], PFChargedObj outch[NTRACK], PFNeutralObj outpho[NPHOTON], PFNeutralObj outne[NSELCALO], PFChargedObj outmu[NMU]) {

    // constants
    const pt_t     TKPT_MAX = PFALGO3_TK_MAXINVPT; // 20 * PT_SCALE;
    const int      DR2MAX   = PFALGO3_DR2MAX_TK_CALO;
    const int      DR2MAX_TM = PFALGO3_DR2MAX_TK_MU;

    ////////////////////////////////////////////////////
    // TK-MU Linking

    // initialize good track bit
    // bool mu_good[NMU];
    // for (int im = 0; im < NMU; ++im) { mu_good[im] = (mu[im].hwPt < TKPT_MAX); }

    // initialize output
    for (int ipf = 0; ipf < NMU; ++ipf) { outmu[ipf].hwPt = 0; outmu[ipf].hwEta = 0; outmu[ipf].hwPhi = 0; outmu[ipf].hwId  = 0; outmu[ipf].hwZ0  = 0; }

    bool isMu[NTRACK];
    for (int it = 0; it < NTRACK; ++it) { isMu[it] = 0; } // initialize
    // for each muon, find the closest track
    for (int im = 0; im < NMU; ++im) {
        if (mu[im].hwPt > 0) {
            pt_t tkPtMin = mu[im].hwPt - 2*(mu[im].hwPtErr);
            int  drmin = DR2MAX_TM, ibest = -1;
            for (int it = 0; it < NTRACK; ++it) {
                if (track[it].hwPt <= tkPtMin) continue;
                int dr = dr2_int(mu[im].hwEta, mu[im].hwPhi, track[it].hwEta, track[it].hwPhi);
                if (dr < drmin) { drmin = dr; ibest = it; }
            }
            if (ibest != -1) {
                outmu[im].hwPt = track[ibest].hwPt;
                outmu[im].hwEta = track[ibest].hwEta;
                outmu[im].hwPhi = track[ibest].hwPhi;
                outmu[im].hwId  = PID_Muon;
                outmu[im].hwZ0 = track[ibest].hwZ0;      
                isMu[ibest] = 1;
            }
        }
    }

    ////////////////////////////////////////////////////
    // TK-EM Linking
    bool isEle[NTRACK];
    HadCaloObj hadcalo_subem[NCALO];
    pfalgo3_em_ref(emcalo, hadcalo, track, isEle, isMu, outpho, hadcalo_subem);

    ////////////////////////////////////////////////////
    // TK-HAD Linking

    // initialize sum track pt
    pt_t calo_sumtk[NCALO], calo_subpt[NCALO];
    int  calo_sumtkErr2[NCALO];
    for (int ic = 0; ic < NCALO; ++ic) { calo_sumtk[ic] = 0;  calo_sumtkErr2[ic] = 0;}

    // initialize good track bit
    bool track_good[NTRACK];
    for (int it = 0; it < NTRACK; ++it) { track_good[it] = (track[it].hwPt < TKPT_MAX || isEle[it] || isMu[it]); }

    // initialize output
    for (int ipf = 0; ipf < NTRACK; ++ipf) { outch[ipf].hwPt = 0; outch[ipf].hwEta = 0; outch[ipf].hwPhi = 0; outch[ipf].hwId = 0; outch[ipf].hwZ0 = 0; }
    for (int ipf = 0; ipf < NSELCALO; ++ipf) { outne[ipf].hwPt = 0; outne[ipf].hwEta = 0; outne[ipf].hwPhi = 0; outne[ipf].hwId = 0; }

    // for each track, find the closest calo
    for (int it = 0; it < NTRACK; ++it) {
        if (track[it].hwPt > 0 && !isEle[it] && !isMu[it]) {
            int  ibest = best_match_with_pt_ref<NCALO,DR2MAX,HadCaloObj>(hadcalo_subem, track[it]);
            //int  ibest = best_match_ref<NCALO,DR2MAX,true,HadCaloObj>(hadcalo_subem, track[it]);
            if (ibest != -1) {
                track_good[it] = 1;
                calo_sumtk[ibest]    += track[it].hwPt;
                calo_sumtkErr2[ibest] += sqr(track[it].hwPtErr);
            }
        }
    }

    for (int ic = 0; ic < NCALO; ++ic) {
        if (calo_sumtk[ic] > 0) {
            pt_t ptdiff = hadcalo_subem[ic].hwPt - calo_sumtk[ic];
            if (ptdiff > 0 && ptdiff*ptdiff > 4*calo_sumtkErr2[ic]) {
                calo_subpt[ic] = ptdiff;
            } else {
                calo_subpt[ic] = 0;
            }
        } else {
            calo_subpt[ic] = hadcalo_subem[ic].hwPt;
        }
    }

    // copy out charged hadrons
    for (int it = 0; it < NTRACK; ++it) {
        if (track_good[it]) {
            outch[it].hwPt = track[it].hwPt;
            outch[it].hwEta = track[it].hwEta;
            outch[it].hwPhi = track[it].hwPhi;
            outch[it].hwZ0 = track[it].hwZ0;
            outch[it].hwId  = isEle[it] ? PID_Electron : (isMu[it] ? PID_Muon : PID_Charged);
        }
    }

    // copy out neutral hadrons
    PFNeutralObj outne_all[NCALO];
    for (int ipf = 0; ipf < NCALO; ++ipf) { outne_all[ipf].hwPt = 0; outne_all[ipf].hwEta = 0; outne_all[ipf].hwPhi = 0; outne_all[ipf].hwId = 0; }
    for (int ic = 0; ic < NCALO; ++ic) {
        if (calo_subpt[ic] > 0) {
            outne_all[ic].hwPt  = calo_subpt[ic];
            outne_all[ic].hwEta = hadcalo_subem[ic].hwEta;
            outne_all[ic].hwPhi = hadcalo_subem[ic].hwPhi;
            outne_all[ic].hwId  = PID_Neutral;
        }
    }

    ptsort_ref<PFNeutralObj,NCALO,NSELCALO>(outne_all, outne);

}
