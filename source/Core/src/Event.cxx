#include "Event.h"

ClassImp(Hit)
ClassImp(Event)

Event::Event() { 
    fHits = new TClonesArray("Hit", 500); 
}

Event::~Event() { 
    delete fHits; 
}

void Event::Clear(Option_t*) {
    fTrueVertices.clear();
    fHits->Clear(); 
}

Hit* Event::AddHit(Hit& newHit) {
    TClonesArray &arr = *fHits;
    int size = arr.GetEntriesFast();
    return new(arr[size]) Hit(newHit);
}

Vertex Event::GetPrimaryVertex() const { 
    if (fTrueVertices.empty()) return Vertex{0.0, 0.0, 0.0, 0}; 
    
    Vertex primary = fTrueVertices[0];
    for (const auto& vtx : fTrueVertices) {
        if (vtx.mult > primary.mult) {
            primary = vtx;
        }
    }
    return primary; 
}

int Event::GetNHits() const { 
    return fHits->GetEntriesFast(); 
}

const Hit* Event::GetHit(int i) const { 
    return (Hit*)fHits->At(i); 
}