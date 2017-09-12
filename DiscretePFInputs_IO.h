#ifndef FASTPUPPI_NTUPLERPRODUCER_DISCRETEPFINPUTS_IO_H
#define FASTPUPPI_NTUPLERPRODUCER_DISCRETEPFINPUTS_IO_H

#include "DiscretePFInputs.h"
#include <vector>
#include <cassert>
#include "src/data.h"

typedef l1tpf_int::InputRegion Region;

struct Event {
	uint32_t run, lumi; uint64_t event;
	float z0;
	float alphaCMed, alphaCRms, alphaFMed, alphaFRms;
	std::vector<Region> regions;
	
	Event() : run(0), lumi(0), event(0), z0(0.), alphaCMed(0.), alphaCRms(0.), alphaFMed(0.), alphaFRms(0.), regions() {}
	bool readFromFile(FILE *fRegionDump) {
		if (!fread(&run, sizeof(uint32_t), 1, fRegionDump)) return false;
		fread(&lumi, sizeof(uint32_t), 1, fRegionDump);
		fread(&event, sizeof(uint64_t), 1, fRegionDump);
		l1tpf_int::readManyFromFile(regions, fRegionDump); 
		fread(&z0, sizeof(float), 1, fRegionDump);
		fread(&alphaCMed, sizeof(float), 1, fRegionDump);
		fread(&alphaCRms, sizeof(float), 1, fRegionDump);
		fread(&alphaFMed, sizeof(float), 1, fRegionDump);
		fread(&alphaFRms, sizeof(float), 1, fRegionDump);
	}
};

class DiscretePFInputs {
	public:
		DiscretePFInputs(const char *fileName) : file_(fopen(fileName,"rb")), iregion_(0) {}
		~DiscretePFInputs() { fclose(file_); }
		bool nextRegion(HadCaloObj calo[NCALO], EmCaloObj emcalo[NEMCALO], TkObj track[NTRACK], MuObj mu[NMU], z0_t & hwZPV) {
			if (!nextRegion()) return false;
		    	const Region &r = event_.regions[iregion_];
			readHadCalo(calo);
			readEmCalo(emcalo);
			readTracks(track, hwZPV);
			readMuons(mu);
			printf("Read region %u with %lu tracks, %lu em calo, %lu had calo, %lu muons\n", iregion_, r.track.size(), r.emcalo.size(), r.calo.size(), r.muon.size());
			iregion_++;
			return true;
		}

	private:
		bool nextRegion() {
			while(true) {
				if (event_.event == 0 || iregion_ == event_.regions.size()) {
					if (feof(file_)) return false;
					if (!event_.readFromFile(file_)) return false;
					printf("Beginning of run %u, lumi %u, event %lu \n", event_.run, event_.lumi, event_.event);
					iregion_ = 0;
				}
				const Region &r = event_.regions[iregion_];
				if (fabs(r.etaCenter) > 1.5) {
					iregion_++;
					continue; // use only regions in the barrel for now
				}
				return true;
			}
		}
		void readTracks(TkObj track[NTRACK], z0_t & hwZPV) {
		    	const Region &r = event_.regions[iregion_];
#ifdef __GXX_EXPERIMENTAL_CXX0X__
			hwZPV = event_.z0 * l1tpf_int::InputTrack::Z0_SCALE;
#else
			hwZPV = event_.z0 * 20;
#endif
			for (unsigned int i = 0; i < std::min<unsigned>(NTRACK,r.track.size()); ++i) {
				track[i].hwPt = r.track[i].hwPt;
				track[i].hwPtErr = r.track[i].hwCaloPtErr;
				track[i].hwEta = r.track[i].hwEta; // @calo
				track[i].hwPhi = r.track[i].hwPhi; // @calo
				track[i].hwZ0 = r.track[i].hwZ0;
			}
		}
		void readOldCalo(CaloObj calo[NCALO]) {
		    	const Region &r = event_.regions[iregion_];
			for (unsigned int i = 0; i < std::min<unsigned>(NCALO, r.calo.size()); ++i) {
				calo[i].hwPt = r.calo[i].hwPt;
				calo[i].hwEta = r.calo[i].hwEta;
				calo[i].hwPhi = r.calo[i].hwPhi;
			}
		}
		void readHadCalo(HadCaloObj calo[NCALO]) {
		    	const Region &r = event_.regions[iregion_];
			for (unsigned int i = 0; i < std::min<unsigned>(NCALO, r.calo.size()); ++i) {
				calo[i].hwPt = r.calo[i].hwPt;
				calo[i].hwEmPt = r.calo[i].hwEmPt;
				calo[i].hwEta = r.calo[i].hwEta;
				calo[i].hwPhi = r.calo[i].hwPhi;
				calo[i].hwIsEM = r.calo[i].isEM;
			}
		}
		void readEmCalo(EmCaloObj calo[NEMCALO]) {
			const Region &r = event_.regions[iregion_];
			for (unsigned int i = 0; i < std::min<unsigned>(NEMCALO, r.emcalo.size()); ++i) {
			    calo[i].hwPt = r.emcalo[i].hwPt;
			    calo[i].hwPtErr = r.emcalo[i].hwPtErr;
			    calo[i].hwEta = r.emcalo[i].hwEta;
			    calo[i].hwPhi = r.emcalo[i].hwPhi;
			}
		}
		void readMuons(MuObj mu[NMU]) {
		    	const Region &r = event_.regions[iregion_];
			for (unsigned int i = 0; i < std::min<unsigned>(NMU,r.muon.size()); ++i) {
				mu[i].hwPt = r.muon[i].hwPt;
				mu[i].hwPtErr = 0; // does not exist in input
				mu[i].hwEta = r.muon[i].hwEta; // @calo
				mu[i].hwPhi = r.muon[i].hwPhi; // @calo
			}
		}

		FILE *file_;
		Event event_;
		unsigned int iregion_;
};
#endif
