//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_BADGUY_PLANT_HPP
#define HEADER_SUPERTUX_BADGUY_PLANT_HPP

#include "badguy/badguy.hpp"

class Plant final : public BadGuy
{
public:
  Plant(const ReaderMapping& reader);

  virtual void initialize() override;
  virtual void collision_solid(const CollisionHit& hit) override;
  virtual HitResponse collision_badguy(BadGuy& badguy, const CollisionHit& hit) override;
  virtual void active_update(float elapsed_time) override;
  virtual void ignite() override;
  virtual std::string get_class() const override {
    return "plant";
  }
  virtual std::string get_display_name() const override {
    return _("Plant");
  }

protected:
  enum PlantState {
    PLANT_SLEEPING,
    PLANT_WAKING,
    PLANT_WALKING
  };

private:
  Timer timer;
  PlantState state;
};

#endif

/* EOF */
