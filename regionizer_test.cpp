#include <cstdio>
#include "firmware/regionizer.h"
#include "firmware/mp7pf_encoding.h"
#include "random_inputs.h"
#include "DiscretePFInputs_IO.h"
#include "pattern_serializer.h"
#include "test_utils.h"

#define NTEST 5

template<unsigned int N, typename T>
bool fill_stream(hls::stream<T> & stream, T data[N], int step=1, int offset=0, const char *name="unknown", int isector=-999) {
    if (!stream.empty()) { printf("ERROR: %s stream for sector %d is not empty\n", name, isector); return false; }
    for (unsigned int i = offset; i < N; i += step) { stream.write(data[i]); }
    return true;
}

void dump_o(FILE *f, const HadCaloObj & obj) { 
    // NOTE: no leading + on positive numbers, VHDL doesn't like it
    fprintf(f, "       %4d % 4d % 4d %3d %1d", int(obj.hwPt), int(obj.hwEta), int(obj.hwPhi), int(obj.hwEmPt), obj.hwIsEM); 
}
void dump_z(FILE *f, const HadCaloObj &) { // the second argument is only to resolve overloading
    HadCaloObj dummy; clear(dummy); dump_o(f,dummy);
}
void dump_o(FILE *f, const TkObj & obj) { 
    // NOTE: no leading + on positive numbers, VHDL doesn't like it
    fprintf(f, "       %4d % 4d % 4d %4d %4d", int(obj.hwPt), int(obj.hwEta), int(obj.hwPhi), int(obj.hwPtErr), int(obj.hwZ0)); 
}
void dump_z(FILE *f, const TkObj &) { // the second argument is only to resolve overloading
    TkObj dummy; clear(dummy); dump_o(f,dummy);
}

int main() {

    // input format: could be random or coming from simulation
    //RandomPFInputs inputs(37); // 37 is a good random number
    DiscretePFInputs inputs("barrel_sectors_1x12_TTbar_PU140.dump");
    HumanReadablePatternSerializer debug("-"); // this will print on stdout, we'll use it for errors

    printf(" --- Configuration --- \n");
    printf(" Sectors: %d \n", N_IN_SECTORS);
    printf("    max N(Calo), input:     %2d \n", NCALO_PER_SECTOR);
    printf("    max N(Calo), eta slice: %2d \n", NCALO_PER_SECTOR_PER_ETA);
    printf("    max N(Track), input:     %2d \n", NTRACK_PER_SECTOR);
    printf("    max N(Track), eta slice: %2d \n", NTRACK_PER_SECTOR_PER_ETA);
    printf(" Regions: %d (%d x %d )\n", N_OUT_REGIONS, N_OUT_REGIONS_ETA, N_OUT_REGIONS_PHI);
    printf("    max N(Calo): %2d \n", NCALO);
    printf("    max N(Track): %2d \n", NTRACK);

    HadCaloObj calo_in[N_IN_SECTORS][NCALO_PER_SECTOR];
    hls::stream<HadCaloObj> calo_fibers[N_IN_SECTORS];
    hls::stream<HadCaloObj> calo_fibers_ref[N_IN_SECTORS];
    HadCaloObj calo_regions[N_OUT_REGIONS][NCALO]; 
    HadCaloObj calo_regions_ref[N_OUT_REGIONS][NCALO]; 

    TkObj track_in[N_IN_SECTORS][NTRACK_PER_SECTOR];
    hls::stream<TkObj> track_fibers[2*N_IN_SECTORS]; // two fibers per sector
    hls::stream<TkObj> track_fibers_ref[2*N_IN_SECTORS];
    TkObj track_regions[N_OUT_REGIONS][NTRACK]; 
    TkObj track_regions_ref[N_OUT_REGIONS][NTRACK]; 


    FILE *f_in  = fopen("dump_in.txt","w");
    FILE *f_out = fopen("dump_out.txt","w");
    int frame_in = 0, frame_out = 0;

#ifdef MP7
    HadCaloObj calo_in_transposed[NCALO_PER_SECTOR][N_IN_SECTORS];
    TkObj track_in_transposed[NTRACK_PER_SECTOR/2][2*N_IN_SECTORS];
    MP7PatternSerializer serMP7_in( "mp7_input.txt",2,1);  
    MP7PatternSerializer serMP7_out("mp7_output.txt",1,0); 
    MP7DataWord mp7_in[MP7_NCHANN];
    MP7DataWord mp7_out[MP7_NCHANN];
#endif

    // -----------------------------------------
    // run multiple tests
    for (int test = 1; test <= NTEST; ++test) {
        // read the event
        if (!inputs.nextEvent()) break;
        if (inputs.event().regions.size() != N_IN_SECTORS) { printf("ERROR: Mismatching number of input regions: %lu\n", inputs.event().regions.size()); return 2; }
        //fill in the streams
        for (int is = 0; is < N_IN_SECTORS; ++is) {
            const Region & r = inputs.event().regions[is];
            // CALO
            dpf2fw::convert<NCALO_PER_SECTOR>(r.calo, calo_in[is]); 
            for (unsigned int i = 0; i < NCALO_PER_SECTOR; ++i) assert(calo_in[is][i].hwPt >= 0);
            if (!fill_stream<NCALO_PER_SECTOR>(calo_fibers[is], calo_in[is], 1, 0, "calo stream", is)) return 3;
            if (!fill_stream<NCALO_PER_SECTOR>(calo_fibers_ref[is], calo_in[is], 1, 0, "calo ref stream", is)) return 3;
            // TRACK
            dpf2fw::convert<NTRACK_PER_SECTOR>(r.track, track_in[is]); 
            for (unsigned int i = 0; i < 2; ++i) {
                if (!fill_stream<NTRACK_PER_SECTOR>(track_fibers[2*is+i],     track_in[is], 2, i, "track stream ",    2*is+i)) return 3;
                if (!fill_stream<NTRACK_PER_SECTOR>(track_fibers_ref[2*is+i], track_in[is], 2, i, "track ref stream", 2*is+i)) return 3;
            }
        }
        // dump inputs
        for (unsigned int ic = 0; ic < N_CLOCKS; ++ic) {
            // takes 2 clocks to send one input; so we just duplicate the lines for now
            int iobj = ic/2; bool send = (ic % 2 == 0);
            fprintf(f_in,"Frame %04d : %2d %2d", ++frame_in, test, iobj);
           // for (int is = 0; is < N_IN_SECTORS; ++is) {
           //     if (iobj < NCALO_PER_SECTOR && send) dump_o(f_in, calo_in[is][iobj]);
           //     else                                 dump_z(f_in, calo_in[is][0]);
           // }
            iobj = ic;
            for (int is = 0; is < N_IN_SECTORS; ++is) {
                if (iobj+0 < NTRACK_PER_SECTOR/2 && send) dump_o(f_in, track_in[is][iobj+0]);
                else                                      dump_z(f_in, track_in[is][0]);
                if (iobj+1 < NTRACK_PER_SECTOR/2 && send) dump_o(f_in, track_in[is][iobj+1]);
                else                                      dump_z(f_in, track_in[is][0]);
            }
            fprintf(f_in,"\n");
        }
#ifdef MP7
        for (int is = 0; is < N_IN_SECTORS; ++is) { for (int io = 0; io < NCALO_PER_SECTOR; ++io) {
            calo_in_transposed[io][is] = calo_in[is][io];
            track_in_transposed[io/2][2*is+(io%2)] = track_in[is][io];
        } }
        for (unsigned int ic = 0; ic < N_CLOCKS/2; ++ic) {
            for (unsigned int i = 0; i < MP7_NCHANN; ++i) mp7_in[i] = 0; // clear
            //if (ic < NCALO_PER_SECTOR) mp7_pack<N_IN_SECTORS,0>(calo_in_transposed[ic], mp7_in);
            if (ic < NTRACK_PER_SECTOR/2) mp7_pack<2*N_IN_SECTORS,0>(track_in_transposed[ic], mp7_in);
            serMP7_in(mp7_in);  
        }
#endif

        // run ref
        regionize_hadcalo(calo_fibers, calo_regions);
        regionize_hadcalo_ref(calo_fibers_ref, calo_regions_ref);
        regionize_track_ref(track_fibers, track_regions); // FIXME: I know both are _ref
        regionize_track_ref(track_fibers_ref, track_regions_ref);

        for (unsigned int ic = 0; ic < N_CLOCKS; ++ic) {
            fprintf(f_out,"Frame %04d :", ++frame_out);
           // for (int i = 0; i < NCALO; ++i) {
           //     if (ic/2 < N_OUT_REGIONS) dump_o(f_out, calo_regions[ic/2][i]);
           //     else                      dump_z(f_out, calo_regions[0 ][i]);
           // }
            for (int i = 0; i < NTRACK; ++i) {
                if (ic/2 < N_OUT_REGIONS) dump_o(f_out, track_regions[ic/2][i]);
                else                      dump_z(f_out, track_regions[0 ][i]);
            }
            fprintf(f_out,"       %d\n", 1);
        }

#ifdef MP7
        for (unsigned int ic = 0; ic < N_CLOCKS; ++ic) {
            for (unsigned int i = 0; i < MP7_NCHANN; ++i) mp7_out[i] = 0; // clear
            //if (ic/2 < N_OUT_REGIONS) mp7_pack<NCALO,0>(calo_regions[ic/2], mp7_out);
            if (ic/2 < N_OUT_REGIONS) mp7_pack<NTRACK,0>(track_regions[ic/2], mp7_out);
            serMP7_out(mp7_out);  
        }
#endif

 
        // -----------------------------------------
        // validation against the reference algorithm
        int errors = 0;
        for (int ir = 0; ir < N_OUT_REGIONS; ++ir) {
            for (int i = 0; i < NCALO; ++i) {
                if (!had_equals(calo_regions_ref[ir][i], calo_regions[ir][i], "regionized had calo", ir*100+i)) { errors++; break; }
            }
            for (int i = 0; i < NTRACK; ++i) {
                if (!track_equals(track_regions_ref[ir][i], track_regions[ir][i], "regionized track", ir*100+i)) { errors++; break; }
            }
        }
 
        int n_expected[N_OUT_REGIONS];
        for (int ir = 0; ir < N_OUT_REGIONS; ++ir) { n_expected[ir] = 0; }
        for (int io = 0; io < NCALO_PER_SECTOR; ++io) {
            for (int is = 0; is < N_IN_SECTORS; ++is) {
                int phi0s = (1 + 2*is - 12)*_PHI_PIO6/2; // pi/12 + is*pi/6 - pi
                if (calo_in[is][io].hwPt == 0) continue;
                for (unsigned int ie = 0; ie < N_OUT_REGIONS_ETA; ++ie) {
                    if (!(ETA_MIN[ie] <= int(calo_in[is][io].hwEta) && int(calo_in[is][io].hwEta) <= ETA_MAX[ie])) continue;
                    for (int ip = 0; ip < N_OUT_REGIONS_PHI; ++ip) {
                        unsigned int ir = N_OUT_REGIONS_PHI*ie + ip;
                        int phi0r = (3 + 4*ip - 12)*_PHI_PIO6/2; // pi/4 + ip*pi/3 - pi // NOTE the offset is chosen so that the boundaries of the region, including the padding, align with the sectors borders
                        int dphi = int(calo_in[is][io].hwPhi) + phi0s - phi0r; 
                        while (dphi >  6*_PHI_PIO6) dphi -= 12*_PHI_PIO6;
                        while (dphi < -6*_PHI_PIO6) dphi += 12*_PHI_PIO6;
                        bool by_cabling = false;
                        for (unsigned int ic = 0; ic < 3; ++ic) if (IN_SECTOR_OF_REGION[ip][ic] == is) by_cabling = true;
                        bool by_dphi = (std::abs(dphi) <= 3*_PHI_PIO6/2);
                        if (by_cabling) n_expected[ir]++;
                        if (by_cabling != by_dphi) {
                            printf("LOGIC ERROR in phi mapping for region %d (iphi %d) : by cabling %d, by dphi %d\n", ir, ip, by_cabling, by_dphi); 
                            printf("object local  iphi in sector: %+6d\n", int(calo_in[is][io].hwPhi)); 
                            printf("object global iphi:           %+6d\n", int(calo_in[is][io].hwPhi) + phi0s); 
                            printf("region global iphi:           %+6d\n", phi0r); 
                            printf("                pi:           %+6d\n", 6*_PHI_PIO6); 
                            printf("              2*pi:           %+6d\n", 12*_PHI_PIO6); 
                            printf("raw     local dphi in region: %+6d\n", int(calo_in[is][io].hwPhi) + phi0s - phi0r); 
                            printf("wrapped local dphi in region: %+6d\n", dphi); 
                            printf("region half size  :           %+6d\n", 3*_PHI_PIO6/2); 
                            printf("object global phi * PI/12:  %+.3f   [-12 to 12 range]\n", (int(calo_in[is][io].hwPhi) + phi0s)/float(_PHI_PIO6/2)); 
                            return 37;
                        }
                        //if (by_cabling) printf("Object %d in sector %d, ieta %+3d (eta %+.3f) extected in region %d (eta %d phi %d)\n", io, is, int(calo_in[is][io].hwEta), calo_in[is][io].hwEta*0.25/_ETA_025, ir, ie, ip);
                    }
                }
            }
        }
        for (int ir = 0; ir < N_OUT_REGIONS; ++ir) { 
            if (std::min<int>(n_expected[ir],NCALO) != count_nonzero(calo_regions_ref[ir], NCALO)) errors++; 
        }

        if (errors != 0) {
            printf("Error in computing test %d (%d)\n", test, errors);
            for (int is = 0; is < N_IN_SECTORS; ++is) {
                printf("INPUT SECTOR %d (FOUND %u): \n", is, count_nonzero(calo_in[is], NCALO_PER_SECTOR));
                debug.dump_hadcalo(calo_in[is], NCALO_PER_SECTOR);
            }
            for (int ir = 0; ir < N_OUT_REGIONS; ++ir) {
                printf("OUTPUT REGION %d (ETA %d PHI %d) (REF; EXPECTED: %u; FOUND %u): \n", ir, ir / N_OUT_REGIONS_PHI, ir % N_OUT_REGIONS_PHI, n_expected[ir], count_nonzero(calo_regions_ref[ir], NCALO));
                debug.dump_hadcalo(calo_regions_ref[ir], NCALO);
                printf("OUTPUT REGION %d (TEST): \n", ir);
                debug.dump_hadcalo(calo_regions[ir], NCALO);
            }
            fclose(f_in); fclose(f_out);
            return 1;
        } else {
            printf("Passed test %d\n", test);
        }

    }
    fclose(f_in); fclose(f_out);
    return 0;
}
