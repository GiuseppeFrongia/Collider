#ifndef EVENT_H
#define EVENT_H

#include <vector>

#include "TObject.h"
#include "TClonesArray.h"

struct Vertex {
    double x, y, z;
    int mult;
};

class Hit : public TObject {
public:
    Hit() = default;
    Hit(const Hit&) = default;
    virtual ~Hit() = default;

    void Set(int layerId, int trackId, double trueR, double trueZ, double truePhi, 
             double measZ, double measPhi, bool isDetected) {
        fLayerID = layerId;
        fTrackID = trackId;
        fTrueR = trueR;
        fTrueZ = trueZ;
        fTruePhi = truePhi;
        fMeasZ = measZ;
        fMeasPhi = measPhi;
        fIsDetected = isDetected;
    }

    bool IsDetected() const { return fIsDetected; }
    double GetTrueR() const { return fTrueR; }
    double GetTrueZ() const { return fTrueZ; }
    double GetTruePhi() const { return fTruePhi; }
    
    double GetMeasZ() const { return fMeasZ; }
    double GetMeasPhi() const { return fMeasPhi; }

    int GetLayerID() const { return fLayerID; }
    int GetTrackID() const { return fTrackID; }

private:
    int fLayerID;
    int fTrackID;
    
    double fTrueR;
    double fTrueZ;
    double fTruePhi;

    double fMeasZ;
    double fMeasPhi;

    bool fIsDetected;
    
    ClassDef(Hit, 1)
};

class Event : public TObject {
public:
    Event();
    virtual ~Event();
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    void Clear(Option_t* option = "") override;
    
    void AddVertex(double x, double y, double z, int mult) { fTrueVertices.push_back(Vertex{x, y, z, mult}); }
    
    Vertex GetPrimaryVertex() const { return fTrueVertices.empty() ? Vertex{0.0,0.0,0.0,0} : fTrueVertices[0]; }
    
    const std::vector<Vertex>& GetVertices() const { return fTrueVertices; }

    Hit* AddHit(Hit& newHit);
    int GetNHits() const;
    const Hit* GetHit(int i) const;

private:
    std::vector<Vertex> fTrueVertices;
    TClonesArray* fHits;
    
    ClassDefOverride(Event, 1)
};

#endif
