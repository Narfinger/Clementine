/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "playlistundocommands.h"
#include "playlist.h"

namespace PlaylistUndoCommands {

Base::Base(Playlist* playlist) : QUndoCommand(0), playlist_(playlist) {}

template <typename PlaylistItemTList>
InsertItems<PlaylistItemTList>::InsertItems(Playlist* playlist, const PlaylistItemTList& items,
                         int pos, bool enqueue)
    : Base(playlist), items_(items), pos_(pos), enqueue_(enqueue) {
  setText(tr("add %n songs", "", items_.size()));
}

template <typename PlaylistItemTList>
void InsertItems<PlaylistItemTList>::redo() {
  playlist_->InsertItemsWithoutUndo(items_, pos_, enqueue_);
}

template <typename PlaylistItemTList>
void InsertItems<PlaylistItemTList>::undo() {
  const int start = pos_ == -1 ? playlist_->rowCount() - items_.size() : pos_;
  playlist_->RemoveItemsWithoutUndo(start, items_.size());
}

template <typename PlaylistItemTList>
bool InsertItems<PlaylistItemTList>::UpdateItem(const PlaylistItemPtr& updated_item) {
  for(auto it = items_.begin(); it != items_.end(); ++it) {
    PlaylistItemPtr item = *it;
    if (item->Metadata().url() == updated_item->Metadata().url()) {
        *it = updated_item;
        return true;
    }
  }
  return false;
}

//define this so we will generate code
template class InsertItems<QList<PlaylistItemPtr> >;
template class InsertItems<std::list<PlaylistItemPtr> >;

RemoveItems::RemoveItems(Playlist* playlist, int pos, int count)
    : Base(playlist) {
  setText(tr("remove %n songs", "", count));

  ranges_ << Range(pos, count);
}

void RemoveItems::redo() {
  for (int i = 0; i < ranges_.count(); ++i)
    ranges_[i].items_ =
        playlist_->RemoveItemsWithoutUndo(ranges_[i].pos_, ranges_[i].count_);
}

void RemoveItems::undo() {
  for (int i = ranges_.count() - 1; i >= 0; --i)
    playlist_->InsertItemsWithoutUndo(ranges_[i].items_, ranges_[i].pos_);
}

bool RemoveItems::mergeWith(const QUndoCommand* other) {
  const RemoveItems* remove_command = static_cast<const RemoveItems*>(other);
  ranges_.append(remove_command->ranges_);

  int sum = 0;
  for (const Range& range : ranges_) sum += range.count_;
  setText(tr("remove %n songs", "", sum));

  return true;
}

MoveItems::MoveItems(Playlist* playlist, const QList<int>& source_rows, int pos)
    : Base(playlist), source_rows_(source_rows), pos_(pos) {
  setText(tr("move %n songs", "", source_rows.count()));
}

void MoveItems::redo() { playlist_->MoveItemsWithoutUndo(source_rows_, pos_); }

void MoveItems::undo() { playlist_->MoveItemsWithoutUndo(pos_, source_rows_); }

ReOrderItems::ReOrderItems(Playlist* playlist,
                           const PlaylistItemList& new_items)
    : Base(playlist), old_items_(playlist->items_), new_items_(new_items) {}

void ReOrderItems::undo() { playlist_->ReOrderWithoutUndo(old_items_); }

void ReOrderItems::redo() { playlist_->ReOrderWithoutUndo(new_items_); }

SortItems::SortItems(Playlist* playlist, int column, Qt::SortOrder order,
                     const PlaylistItemList& new_items)
    : ReOrderItems(playlist, new_items), column_(column), order_(order) {
  setText(tr("sort songs"));
}

ShuffleItems::ShuffleItems(Playlist* playlist,
                           const PlaylistItemList& new_items)
    : ReOrderItems(playlist, new_items) {
  setText(tr("shuffle songs"));
}
}  // namespace


