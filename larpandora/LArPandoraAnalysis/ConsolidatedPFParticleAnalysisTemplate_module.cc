/**
 *  @file   larpandora/LArPandoraAnalysis/ConsolidatedPFParticleAnalysisTemplate_module.cc
 *
 *  @brief  A template analysis module for using the Pandora consolidated output
 */

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"

#include "larpandora/LArPandoraObjects/PFParticleMetadata.h"

#include <string>

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_pandora
{

/**
 *  @brief  ConsolidatedPFParticleAnalysisTemplate class
 */
class ConsolidatedPFParticleAnalysisTemplate : public art::EDAnalyzer
{
public:
    typedef art::Handle< std::vector<recob::PFParticle> > PFParticleHandle;
    typedef std::map< size_t, art::Ptr<recob::PFParticle> > PFParticleIdMap;
    typedef std::vector< art::Ptr<larpandoraobj::PFParticleMetadata> > PFParticleMetadataVector;
    typedef std::map< art::Ptr<recob::PFParticle>, PFParticleMetadataVector > PFParticleMetadataMap;
    typedef std::vector< art::Ptr<recob::PFParticle> > PFParticleVector;
    typedef std::vector< art::Ptr<recob::Track> > TrackVector;
    typedef std::vector< art::Ptr<recob::Shower> > ShowerVector;

    /**
     *  @brief  Constructor
     *
     *  @param  pset the set of input fhicl parameters
     */
    ConsolidatedPFParticleAnalysisTemplate(fhicl::ParameterSet const &pset);
    
    /**
     *  @brief  Configure memeber variables using FHiCL parameters
     *
     *  @param  pset the set of input fhicl parameters
     */
    void reconfigure(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Analyze an event!
     *
     *  @param  evt the art event to analyze
     */
    void analyze(const art::Event &evt);

private:
    /**
     *  @brief  Produce a mapping from PFParticle ID to the art ptr to the PFParticle itself for fast navigation
     *
     *  @param  pfParticleHandle the handle for the PFParticle collection
     *  @param  pfParticleMap the mapping from ID to PFParticle
     */
    void GetPFParticleIdMap(const PFParticleHandle &pfParticleHandle, PFParticleIdMap &pfParticleMap) const;

    /**
     *  @brief  Produce a mapping from PFParticle to a vector of art ptr to PFParticleMetadata objects for fast navigation
     *
     *  @param  evt the art event to analyze
     *  @param  pfParticleHandle the handle for the PFParticle collection
     *  @param  pfParticleMetadataMap the mapping from PFParticle to PFParticleMetadata objects
     */
    void GetPFParticleMetadataMap(const art::Event &evt, const PFParticleHandle &pfParticleHandle, PFParticleMetadataMap &pfParticleMetadataMap) const;

    /**
     *  @brief Print out scores in PFParticleMetadata
     *
     *  @param evt the art event to analyze
     *  @param pfParticleHandle the handle for the PFParticle collection
     */
    void PrintOutScores(const PFParticleMetadataMap &pfParticleMetadataMap) const;

    /**
     *  @brief  Produce a mapping from PFParticle ID to the art ptr to the PFParticle itself for fast navigation
     *
     *  @param  pfParticleMap the mapping from ID to PFParticle
     *  @param  pfParticleMetadataMap the mapping from PFParticle to PFParticleMetadata objects
     *  @param  crParticles a vector to hold the top-level PFParticles reconstructed under the cosmic hypothesis
     *  @param  nuParticles a vector to hold the final-states of the reconstruced neutrino
     */
    void GetFinalStatePFParticleVectors(const PFParticleIdMap &pfParticleMap, const PFParticleMetadataMap &pfParticleMetadataMap, PFParticleVector &crParticles, PFParticleVector &nuParticles) const;

    /**
     *  @brief  Determine whether PFParticle is target
     *
     *  @param  pParticle in question
     *  @param  pfParticleMetadataMap the mapping from PFParticle to PFParticleMetadata objects
     */
    bool IsTarget(const art::Ptr<recob::PFParticle> pParticle, const PFParticleMetadataVector &pfParticleMetadataVector) const;

    /**
     *  @brief  Collect associated tracks and showers to particles in an input particle vector
     *
     *  @param  particles a vector holding PFParticles from which to find the associated tracks and showers
     *  @param  pfParticleHandle the handle for the PFParticle collection
     *  @param  evt the art event to analyze
     *  @param  tracks a vector to hold the associated tracks
     *  @param  showers a vector to hold the associated showers
     */
    void CollectTracksAndShowers(const PFParticleVector &particles, const PFParticleHandle &pfParticleHandle, const art::Event &evt, TrackVector &tracks, ShowerVector &showers);

    std::string m_pandoraLabel;         ///< The label for the pandora producer
    std::string m_trackLabel;           ///< The label for the track producer from PFParticles
    std::string m_showerLabel;          ///< The label for the shower producer from PFParticles
    bool        m_printOutScores;       ///< Option to investigate the associations to scores for PFParticles
    bool        m_testBeamMode;         ///< Option to run the module in test beam mode
};

DEFINE_ART_MODULE(ConsolidatedPFParticleAnalysisTemplate)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "Pandora/PdgTable.h"

#include <iostream>

namespace lar_pandora
{

ConsolidatedPFParticleAnalysisTemplate::ConsolidatedPFParticleAnalysisTemplate(fhicl::ParameterSet const &pset) : art::EDAnalyzer(pset)
{
    this->reconfigure(pset);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::reconfigure(fhicl::ParameterSet const &pset)
{
    m_pandoraLabel = pset.get<std::string>("PandoraLabel");
    m_trackLabel = pset.get<std::string>("TrackLabel");
    m_showerLabel = pset.get<std::string>("ShowerLabel");
    m_printOutScores = pset.get<bool>("PrintOutScores",true);
    m_testBeamMode = pset.get<bool>("TestBeamMode",false);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::analyze(const art::Event &evt)
{
    // Collect the PFParticles from the event
    PFParticleHandle pfParticleHandle;
    evt.getByLabel(m_pandoraLabel, pfParticleHandle);
   
    if (!pfParticleHandle.isValid())
    {
        mf::LogDebug("ConsolidatedPFParticleAnalysisTemplate") << "  Failed to find the PFParticles." << std::endl;
        return;
    }

    // Produce a map of the PFParticle IDs for fast navigation through the hierarchy
    PFParticleIdMap pfParticleMap;
    PFParticleMetadataMap pfParticleMetadataMap;
    this->GetPFParticleIdMap(pfParticleHandle, pfParticleMap);
    this->GetPFParticleMetadataMap(evt, pfParticleHandle, pfParticleMetadataMap);    

    /// Investigate scores associated as larpandoraobject::metadata for the PFParticles
    if (m_printOutScores)
        this->PrintOutScores(pfParticleMetadataMap);

    // Produce two PFParticle vectors containing final-state particles:
    // 1. Particles identified as cosmic-rays - recontructed under cosmic-hypothesis
    // 2. Daughters of the neutrino or test beam PFParticle - reconstructed under the neutrino hypothesis
    std::vector< art::Ptr<recob::PFParticle> > crParticles;
    std::vector< art::Ptr<recob::PFParticle> > nuParticles;
    this->GetFinalStatePFParticleVectors(pfParticleMap, pfParticleMetadataMap, crParticles, nuParticles);

    // Use as required!
    // -----------------------------
    //   What follows is an example showing how one might access the reconstructed neutrino final-state tracks and showers
    
    // These are the vectors to hold the tracks and showers for the final-states of the reconstructed neutrino
    std::vector< art::Ptr<recob::Track> > tracks;
    std::vector< art::Ptr<recob::Shower> > showers;
    this->CollectTracksAndShowers(nuParticles, pfParticleHandle, evt, tracks, showers);

    // Print a summary of the consolidated event
    std::cout << "Consolidated event summary:" << std::endl;
    std::cout << "  - Number of primary cosmic-ray PFParticles   : " << crParticles.size() << std::endl;
    std::cout << "  - Number of " << (m_testBeamMode ? "test beam" : "neutrino") << " final-state PFParticles : " << nuParticles.size() << std::endl;
    std::cout << "    ... of which are track-like   : " << tracks.size() << std::endl;
    std::cout << "    ... of which are showers-like : " << showers.size() << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::GetPFParticleIdMap(const PFParticleHandle &pfParticleHandle, PFParticleIdMap &pfParticleMap) const
{
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pParticle(pfParticleHandle, i);

        if (!pfParticleMap.insert(PFParticleIdMap::value_type(pParticle->Self(), pParticle)).second)
            throw cet::exception("ConsolidatedPFParticleAnalysisTemplate") << "  Unable to get PFParticle ID map, the input PFParticle collection has repeat IDs!";
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::GetPFParticleMetadataMap(const art::Event &evt, const PFParticleHandle &pfParticleHandle, PFParticleMetadataMap &pfParticleMetadataMap) const
{
    // Get the associations between PFParticles and larpandoraobj::PFParticleMetadata
    art::FindManyP< larpandoraobj::PFParticleMetadata > pfPartToMetadataAssoc(pfParticleHandle, evt, m_pandoraLabel);

    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const std::vector< art::Ptr<larpandoraobj::PFParticleMetadata> > &pfParticleMetadataList(pfPartToMetadataAssoc.at(i));
        if (!pfParticleMetadataList.empty())
        {
            const art::Ptr<recob::PFParticle> pParticle(pfParticleHandle, i);

            if (!pfParticleMetadataMap.insert(PFParticleMetadataMap::value_type(pParticle, pfParticleMetadataList)).second)
                throw cet::exception("ConsolidatedPFParticleAnalysisTemplate") << "  Unable to get PFParticle Metadata map, the input PFParticle appears twice!";
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ConsolidatedPFParticleAnalysisTemplate::PrintOutScores(const PFParticleMetadataMap &pfParticleMetadataMap) const
{
    for (const auto &pfParticleToMetaDataIter : pfParticleMetadataMap)
    {
        const art::Ptr<recob::PFParticle> pParticle(pfParticleToMetaDataIter.first);
        const std::vector< art::Ptr<larpandoraobj::PFParticleMetadata> > &pfParticleMetadataList(pfParticleToMetaDataIter.second);

        if (!pfParticleMetadataList.empty())
        {
            for (unsigned int j = 0; j < pfParticleMetadataList.size(); ++j)
            {
                const art::Ptr<larpandoraobj::PFParticleMetadata> &pfParticleMetadata(pfParticleMetadataList.at(j));
                const pandora::PropertiesMap &pfParticlePropertiesMap(pfParticleMetadata->GetPropertiesMap());

                if (!pfParticlePropertiesMap.empty())
                    std::cout << " Found PFParticle " << pParticle->Self() << " with: " << std::endl;

                for (pandora::PropertiesMap::const_iterator it = pfParticlePropertiesMap.begin(); it != pfParticlePropertiesMap.end(); ++it)
                    std::cout << "  - " << it->first << " = " << it->second << std::endl;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
    
void ConsolidatedPFParticleAnalysisTemplate::GetFinalStatePFParticleVectors(const PFParticleIdMap &pfParticleMap, const PFParticleMetadataMap &pfParticleMetadataMap, PFParticleVector &crParticles, PFParticleVector &nuParticles) const
{
    for (PFParticleIdMap::const_iterator it = pfParticleMap.begin(); it != pfParticleMap.end(); ++it)
    {
        const art::Ptr<recob::PFParticle> pParticle(it->second);

        // Only look for primary particles
        if (!pParticle->IsPrimary()) continue;

        // Check if this particle is identified as the neutrino or a test beam particle
        const bool isTarget(this->IsTarget(pParticle, pfParticleMetadataMap.at(pParticle)));

        // All non-neutrino primary particles are reconstructed under the cosmic hypothesis
        if (!isTarget)
        {
            crParticles.push_back(pParticle);
            continue;
        }

        // ATTN. We are filling nuParticles under the assumption that there is only one reconstructed neutrino identified per event.
        //       If this is not the case please handle accordingly
        if (!nuParticles.empty() && !m_testBeamMode)
        {
            throw cet::exception("ConsolidatedPFParticleAnalysisTemplate") << "  This event contains multiple reconstructed neutrinos!";
        }

        // Add the daughters of the PFParticle to the nuPFParticles vector
        for (const size_t daughterId : pParticle->Daughters())
        {
            if (pfParticleMap.find(daughterId) == pfParticleMap.end())
                throw cet::exception("ConsolidatedPFParticleAnalysisTemplate") << "  Invalid PFParticle collection!";

            nuParticles.push_back(pfParticleMap.at(daughterId));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

bool ConsolidatedPFParticleAnalysisTemplate::IsTarget(const art::Ptr<recob::PFParticle> pParticle, const PFParticleMetadataVector &pfParticleMetadataVector) const
{
    const std::string property(m_testBeamMode ? "IsTestBeam" : "IsNeutrino");

    if (!pfParticleMetadataVector.empty())
    {
        for (unsigned int j = 0; j < pfParticleMetadataVector.size(); ++j)
        {
            const art::Ptr<larpandoraobj::PFParticleMetadata> &pfParticleMetadata(pfParticleMetadataVector.at(j));
            const pandora::PropertiesMap &pfParticlePropertiesMap(pfParticleMetadata->GetPropertiesMap());

            pandora::PropertiesMap::const_iterator iter(pfParticlePropertiesMap.find(property));

            if (iter != pfParticlePropertiesMap.end())
                return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------------------------------------------------
    
void ConsolidatedPFParticleAnalysisTemplate::CollectTracksAndShowers(const PFParticleVector &particles, const PFParticleHandle &pfParticleHandle, const art::Event &evt, TrackVector &tracks, ShowerVector &showers)
{
    // Get the associations between PFParticles and tracks/showers from the event
    art::FindManyP< recob::Track > pfPartToTrackAssoc(pfParticleHandle, evt, m_trackLabel);
    art::FindManyP< recob::Shower > pfPartToShowerAssoc(pfParticleHandle, evt, m_showerLabel);
   
    for (const art::Ptr<recob::PFParticle> &pParticle : particles)
    {
        const std::vector< art::Ptr<recob::Track> > associatedTracks(pfPartToTrackAssoc.at(pParticle.key()));
        const std::vector< art::Ptr<recob::Shower> > associatedShowers(pfPartToShowerAssoc.at(pParticle.key()));
        const unsigned int nTracks(associatedTracks.size());
        const unsigned int nShowers(associatedShowers.size());

        // Check if the PFParticle has no associated tracks or showers
        if (nTracks == 0 && nShowers == 0)
        {
            mf::LogDebug("ConsolidatedPFParticleAnalysisTemplate") << "  No tracks or showers were associated to PFParticle " << pParticle->Self() << std::endl;
            continue;
        }

        // Check if there is an associated track
        if (nTracks == 1 && nShowers == 0)
        {
            tracks.push_back(associatedTracks.front());
            continue;
        }

        // Check if there is an associated shower
        if (nTracks == 0 && nShowers == 1)
        {
            showers.push_back(associatedShowers.front());
            continue;
        }

        throw cet::exception("ConsolidatedPFParticleAnalysisTemplate") << "  There were " << nTracks << " tracks and " << nShowers << " showers associated with PFParticle " << pParticle->Self();
    }
}

} //namespace lar_pandora
