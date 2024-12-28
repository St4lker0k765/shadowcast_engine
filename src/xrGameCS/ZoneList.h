#pragma once

#include "DetectList.h"
#include "CustomZone.h"

class CZoneList : public CDetectList<CCustomZone>
{
protected:
    virtual bool feel_touch_contact(CObject* O);

public:
    CZoneList();
    virtual ~CZoneList();
};