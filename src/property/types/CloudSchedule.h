//
// This file is part of ArduinoCloudThing
//
// Copyright 2021 ARDUINO SA (http://www.arduino.cc/)
//
// This software is released under the GNU General Public License version 3,
// which covers the main part of ArduinoCloudThing.
// The terms of this license can be found at:
// https://www.gnu.org/licenses/gpl-3.0.en.html
//
// You can be released from the requirements of the above licenses by purchasing
// a commercial license. Buying such a license is mandatory if you want to modify or
// otherwise use the software for commercial activities involving the Arduino
// software without disclosing the source code of your own applications. To purchase
// a commercial license, send an email to license@arduino.cc.
//

#ifndef CLOUDSCHEDULE_H_
#define CLOUDSCHEDULE_H_

/******************************************************************************
   INCLUDE
 ******************************************************************************/

#include <Arduino.h>
#include "../Property.h"
#include "../../AIoTC_Const.h"
#include "utility/time/TimeService.h"
#include <time.h>

/******************************************************************************
 * DEFINE
 ******************************************************************************/
#define SCHEDULE_UNIT_MASK    0xC0000000
#define SCHEDULE_UNIT_SHIFT   30

#define SCHEDULE_TYPE_MASK    0x3C000000
#define SCHEDULE_TYPE_SHIFT   26

#define SCHEDULE_MONTH_MASK   0x0000FF00
#define SCHEDULE_MONTH_SHIFT  8

#define SCHEDULE_REP_MASK     0x03FFFFFF
#define SCHEDULE_WEEK_MASK    0x000000FF
#define SCHEDULE_DAY_MASK     0x000000FF

#define SCHEDULE_ONE_SHOT     0xFFFFFFFF

/******************************************************************************
   ENUM
 ******************************************************************************/
enum class ScheduleUnit : int {
  Seconds      = 0,
  Minutes      = 1,
  Hours        = 2,
  Days         = 3
};

enum class ScheduleType : int {
  OneShot      = 0,
  FixedDelta   = 1,
  Weekly       = 2,
  Monthly      = 3,
  Yearly       = 4
};

/******************************************************************************
   CLASS DECLARATION
 ******************************************************************************/
class Schedule : public TimeService {
  public:
    unsigned int frm, to, len, msk;
    Schedule(unsigned int s, unsigned int e, unsigned int d, unsigned int m): frm(s), to(e), len(d), msk(m) {}

    bool isActive() {

      unsigned int now = getLocalTime();
      if(checkSchedulePeriod(now, frm, to)) {
        /* We are in the schedule range */

        if(checkScheduleMask(now, msk)) {
        
          /* We can assume now that the schedule is always repeating with fixed delta */ 
          unsigned int delta = getScheduleDelta(msk);
          if ( ( (std::max(now , frm) - std::min(now , frm)) % delta ) <= len ) {
            return true;
          }
        }
      }
      return false;
    }

    Schedule& operator=(Schedule & aSchedule) {
      frm = aSchedule.frm;
      to  = aSchedule.to;
      len = aSchedule.len;
      msk = aSchedule.msk;
      return *this;
    }

    bool operator==(Schedule & aSchedule) {
      return frm == aSchedule.frm && to == aSchedule.to && len == aSchedule.len && msk == aSchedule.msk;
    }

    bool operator!=(Schedule & aSchedule) {
      return !(operator==(aSchedule));
    }
  private:
    ScheduleUnit getScheduleUnit(unsigned int msk) {
      return static_cast<ScheduleUnit>((msk & SCHEDULE_UNIT_MASK) >> SCHEDULE_UNIT_SHIFT);
    }

    ScheduleType getScheduleType(unsigned int msk) {
      return static_cast<ScheduleType>((msk & SCHEDULE_TYPE_MASK) >> SCHEDULE_TYPE_SHIFT);
    }

    unsigned int getScheduleRepetition(unsigned int msk) {
      return (msk & SCHEDULE_REP_MASK);
    }

    unsigned int getScheduleWeekMask(unsigned int msk) {
      return (msk & SCHEDULE_WEEK_MASK);
    }

    unsigned int getScheduleDay(unsigned int msk) {
      return (msk & SCHEDULE_DAY_MASK);
    }

    unsigned int getScheduleMonth(unsigned int msk) {
      return ((msk & SCHEDULE_MONTH_MASK) >> SCHEDULE_MONTH_SHIFT);
    }

    bool isScheduleOneShot(unsigned int msk) {
      return (getScheduleType(msk) == ScheduleType::OneShot) ? true : false ;
    }

    bool isScheduleFixed(unsigned int msk) {
      return (getScheduleType(msk) == ScheduleType::FixedDelta) ? true : false ;
    }

    bool isScheduleWeekly(unsigned int msk) {
      return (getScheduleType(msk) == ScheduleType::Weekly) ? true : false ;
    }

    bool isScheduleMonthly(unsigned int msk) {
      return (getScheduleType(msk) == ScheduleType::Monthly) ? true : false ;
    }

    bool isScheduleYearly(unsigned int msk) {
      return (getScheduleType(msk) == ScheduleType::Yearly) ? true : false ;
    }

    bool isScheduleInSeconds(unsigned int msk) {
      if(isScheduleFixed(msk)) {
        return (getScheduleUnit(msk) == ScheduleUnit::Seconds) ? true : false ;
      } else {
        return false;
      }
    }

    bool isScheduleInMinutes(unsigned int msk) {
      if(isScheduleFixed(msk)) {
        return (getScheduleUnit(msk) == ScheduleUnit::Minutes) ? true : false ;
      } else {
        return false;
      }
    }

    bool isScheduleInHours(unsigned int msk) {
      if(isScheduleFixed(msk)) {
        return (getScheduleUnit(msk) == ScheduleUnit::Hours) ? true : false ;
      } else {
        return false;
      }
    }

    bool isScheduleInDays(unsigned int msk) {
      if(isScheduleFixed(msk)) {
        return (getScheduleUnit(msk) == ScheduleUnit::Days) ? true : false ;
      } else {
        return false;
      }
    }

    unsigned int getCurrentDayMask(time_t time) {
      struct tm * ptm;
      ptm = gmtime (&time);

      return 1 << ptm->tm_wday;
    }

    unsigned int getCurrentDay(time_t time) {
      struct tm * ptm;
      ptm = gmtime (&time);

      return ptm->tm_mday;
    }

    unsigned int getCurrentMonth(time_t time) {
      struct tm * ptm;
      ptm = gmtime (&time);

      return ptm->tm_mon;
    }

    bool checkSchedulePeriod(unsigned int now, unsigned int frm, unsigned int to) {
      /* Check if current time is inside the schedule period. If 'to' is equal to
       * 0 the schedule has no end.
       */
      if(now >= frm && (now < to || to == 0)) {
        return true;
      } else {
        return false;
      }
    }

    bool checkScheduleMask(unsigned int now, unsigned int msk) {
      if(isScheduleFixed(msk) || isScheduleOneShot(msk)) {
        return true;
      }
      
      if(isScheduleWeekly(msk)) {
        unsigned int currentDayMask = getCurrentDayMask(now);
        unsigned int scheduleMask = getScheduleWeekMask(msk);
        
        if((currentDayMask & scheduleMask) != 0) {
          return true;
        }
      }

      if(isScheduleMonthly(msk)) {
        unsigned int currentDay = getCurrentDay(now);
        unsigned int scheduleDay = getScheduleDay(msk);

        if(currentDay == scheduleDay) {
          return true;
        }
      }

      if(isScheduleYearly(msk)) {
        unsigned int currentDay = getCurrentDay(now);
        unsigned int scheduleDay = getScheduleDay(msk);
        unsigned int currentMonth = getCurrentMonth(now);
        unsigned int scheduleMonth = getScheduleMonth(msk);

        if((currentDay == scheduleDay) && (currentMonth == scheduleMonth)) {
          return true;
        }
      }

      return false;
    }

    unsigned int getScheduleDelta(unsigned int msk) {
      if(isScheduleInSeconds(msk)) {
        return SECONDS * getScheduleRepetition(msk);
      }

      if(isScheduleInMinutes(msk)) {
        return MINUTES * getScheduleRepetition(msk);
      }

      if(isScheduleInHours(msk)) {
        return HOURS * getScheduleRepetition(msk);
      }

      if(isScheduleInDays(msk)) {
        return DAYS * getScheduleRepetition(msk);
      }

      if(isScheduleWeekly(msk) || isScheduleMonthly(msk) || isScheduleYearly(msk)) {
        return DAYS;
      }

      return SCHEDULE_ONE_SHOT;
    }
};

class CloudSchedule : public Property {
  private:
    Schedule _value,
             _cloud_value;
  public:
    CloudSchedule() : _value(0, 0, 0, 0), _cloud_value(0, 0, 0, 0) {}
    CloudSchedule(unsigned int frm, unsigned int to, unsigned int len, unsigned int msk) : _value(frm, to, len, msk), _cloud_value(frm, to, len, msk) {}

    virtual bool isDifferentFromCloud() {

      return _value != _cloud_value;
    }

    CloudSchedule& operator=(Schedule aSchedule) {
      _value.frm = aSchedule.frm;
      _value.to  = aSchedule.to;
      _value.len = aSchedule.len;
      _value.msk = aSchedule.msk;
      updateLocalTimestamp();
      return *this;
    }

    Schedule getCloudValue() {
      return _cloud_value;
    }

    Schedule getValue() {
      return _value;
    }

    bool isActive() {
      return _value.isActive();
    }

    virtual void fromCloudToLocal() {
      _value = _cloud_value;
    }
    virtual void fromLocalToCloud() {
      _cloud_value = _value;
    }
    virtual CborError appendAttributesToCloud() {
      CHECK_CBOR(appendAttribute(_value.frm));
      CHECK_CBOR(appendAttribute(_value.to));
      CHECK_CBOR(appendAttribute(_value.len));
      CHECK_CBOR(appendAttribute(_value.msk));
      return CborNoError;
    }
    virtual void setAttributesFromCloud() {
      setAttribute(_cloud_value.frm);
      setAttribute(_cloud_value.to);
      setAttribute(_cloud_value.len);
      setAttribute(_cloud_value.msk);
    }
};

#endif /* CLOUDSCHEDULE_H_ */
