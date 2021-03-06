//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
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

#include "editor/layers_gui.hpp"

#include "editor/editor.hpp"
#include "editor/layer_icon.hpp"
#include "editor/object_menu.hpp"
#include "editor/tip.hpp"
#include "gui/menu_manager.hpp"
#include "math/vector.hpp"
#include "object/camera.hpp"
#include "object/tilemap.hpp"
#include "supertux/colorscheme.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "supertux/resources.hpp"
#include "supertux/sector.hpp"
#include "video/drawing_context.hpp"
#include "video/renderer.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

EditorLayersGui::EditorLayersGui() :
  layers(),
  selected_tilemap(),
  Ypos(448),
  Width(512),
  sector_text(),
  sector_text_width(0),
  hovered_item(HI_NONE),
  hovered_layer(-1),
  object_tip()
{
}

void
EditorLayersGui::draw(DrawingContext& context) {

  if (object_tip) {
    auto position = get_layer_coords(hovered_layer);
    object_tip->draw_up(context, position);
  }

  context.color().draw_filled_rect(Rectf(Vector(0, static_cast<float>(Ypos)), Vector(static_cast<float>(Width), static_cast<float>(SCREEN_HEIGHT))),
                                     Color(0.9f, 0.9f, 1.0f, 0.6f),
                                     0.0f,
                                     LAYER_GUI-10);

  Rectf target_rect = Rectf(0, 0, 0, 0);
  bool draw_rect = true;
  switch (hovered_item) {
    case HI_SPAWNPOINTS:
      target_rect = Rectf(Vector(0, static_cast<float>(Ypos)), Vector(static_cast<float>(Xpos), static_cast<float>(SCREEN_HEIGHT)));
      break;
    case HI_SECTOR:
      target_rect = Rectf(Vector(static_cast<float>(Xpos), static_cast<float>(Ypos)),
                          Vector(static_cast<float>(sector_text_width + Xpos), static_cast<float>(SCREEN_HEIGHT)));
      break;
    case HI_LAYERS: {
      Vector coords = get_layer_coords(hovered_layer);
      target_rect = Rectf(coords, coords + Vector(32, 32));
    } break;
    default:
      draw_rect = false;
      break;
  }
  if(draw_rect)
  {
    context.color().draw_filled_rect(target_rect, Color(0.9f, 0.9f, 1.0f, 0.6f), 0.0f,
                                       LAYER_GUI-5);
  }

  if (!Editor::current()->is_level_loaded()) {
    return;
  }

  context.color().draw_text(Resources::normal_font, sector_text,
                            Vector(35.0f, static_cast<float>(Ypos) + 5.0f),
                            ALIGN_LEFT, LAYER_GUI, ColorScheme::Menu::default_color);

  int pos = 0;
  for(const auto& layer_icon : layers) {
    if (layer_icon->is_valid()) {
      layer_icon->draw(context, get_layer_coords(pos));
    }
    pos++;
  }
}

void
EditorLayersGui::update(float elapsed_time) {
  auto it = layers.begin();
  while(it != layers.end())
  {
    auto layer_icon = (*it).get();
    if(!layer_icon->is_valid())
      it = layers.erase(it);
    else
      ++it;
  }
}

bool
EditorLayersGui::event(SDL_Event& ev) {
  auto editor = Editor::current();
  switch (ev.type) {
    case SDL_MOUSEBUTTONDOWN:
    {
      if (ev.button.button == SDL_BUTTON_LEFT) {
        switch (hovered_item) {
          case HI_SECTOR:
            editor->disable_keyboard();
            MenuManager::instance().set_menu(MenuStorage::EDITOR_SECTORS_MENU);
            break;
          case HI_LAYERS:
            if (hovered_layer >= layers.size()) {
              break;
            }
            if ( layers[hovered_layer]->is_tilemap ) {
              if (selected_tilemap) {
                (static_cast<TileMap*>(selected_tilemap))->m_editor_active = false;
              }
              selected_tilemap = layers[hovered_layer]->layer;
              (static_cast<TileMap*>(selected_tilemap))->m_editor_active = true;
              editor->edit_path((static_cast<TileMap*>(selected_tilemap))->get_path(),
                                                       selected_tilemap);
            } else {
              auto cam = dynamic_cast<Camera*>(layers[hovered_layer]->layer);
              if (cam) {
                editor->edit_path(cam->get_path(), cam);
              }
            }
            break;
          default:
            return false;
            break;
        }
      } else if (ev.button.button == SDL_BUTTON_RIGHT) {
        if (hovered_item == HI_LAYERS && hovered_layer < layers.size()) {
          auto om = std::make_unique<ObjectMenu>(layers[hovered_layer]->layer);
          editor->deactivate_request = true;
          MenuManager::instance().push_menu(move(om));
        } else {
          return false;
        }
      }
    } break;

    case SDL_MOUSEMOTION:
    {
      Vector mouse_pos = VideoSystem::current()->get_viewport().to_logical(ev.motion.x, ev.motion.y);
      float x = mouse_pos.x - static_cast<float>(Xpos);
      float y = mouse_pos.y - static_cast<float>(Ypos);
      if (y < 0 || x > static_cast<float>(Width)) {
        hovered_item = HI_NONE;
        object_tip = nullptr;
        return false;
      }
      if (x < 0) {
        hovered_item = HI_SPAWNPOINTS;
        object_tip = nullptr;
        break;
      } else {
        if (x <= static_cast<float>(sector_text_width)) {
          hovered_item = HI_SECTOR;
          object_tip = nullptr;
        } else {
          unsigned int new_hovered_layer = get_layer_pos(mouse_pos);
          if (hovered_layer != new_hovered_layer || hovered_item != HI_LAYERS) {
            hovered_layer = new_hovered_layer;
            update_tip();
          }
          hovered_item = HI_LAYERS;
        }
      }
    }
    break;

    case SDL_WINDOWEVENT:
      if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
        resize();
      }
      return false;
    default:
      return false;
  }
  return true;
}

void
EditorLayersGui::resize() {
  Ypos = SCREEN_HEIGHT - 32;
  Width = SCREEN_WIDTH - 128;
}

void
EditorLayersGui::setup() {
  resize();
}

void
EditorLayersGui::refresh_sector_text() {
  sector_text = _("Sector") + ": " + Editor::current()->currentsector->get_name();
  sector_text_width  = int(Resources::normal_font->get_text_width(sector_text)) + 6;
}

void
EditorLayersGui::sort_layers() {
  std::sort(layers.begin(), layers.end(), less_z_pos);
}

void
EditorLayersGui::add_layer(GameObject* layer) {
  auto icon = std::make_unique<LayerIcon>(layer);
  int z_pos = icon->get_zpos();

  // The icon is inserted to the correct position.
  for(auto i = layers.begin(); i != layers.end(); ++i) {
    const auto& li = i->get();
    if (li->get_zpos() < z_pos) {
      layers.insert(i, move(icon));
      return;
    }
  }
  layers.push_back(move(icon));
}

void
EditorLayersGui::update_tip() {
  if ( hovered_layer >= layers.size() ) {
    object_tip = nullptr;
    return;
  }
  auto new_tip = std::make_unique<Tip>(layers[hovered_layer]->layer);
  object_tip = move(new_tip);
}

Vector
EditorLayersGui::get_layer_coords(const int pos) const {
  return Vector(static_cast<float>(pos * 35 + Xpos + sector_text_width),
                static_cast<float>(Ypos));
}

int
EditorLayersGui::get_layer_pos(const Vector& coords) const {
  return static_cast<int>((coords.x - static_cast<float>(Xpos) - static_cast<float>(sector_text_width)) / 35.0f);
}

bool
EditorLayersGui::less_z_pos(const std::unique_ptr<LayerIcon>& lhs, const std::unique_ptr<LayerIcon>& rhs) {
  return lhs->get_zpos() < rhs->get_zpos();
}

/* EOF */
