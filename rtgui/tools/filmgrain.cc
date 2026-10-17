/** -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "filmgrain.h"

#include "eventmapper.h"

#include "rtengine/procparams.h"

using namespace rtengine;
using namespace rtengine::procparams;

const Glib::ustring FilmGrain::TOOL_NAME = "filmgrain";

FilmGrain::FilmGrain(): FoldableToolPanel(this, TOOL_NAME, M("TP_FILMGRAIN_LABEL"), false, true)
{
    auto m = ProcEventMapper::getInstance();
    EvFilmGrainEnabled = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_ENABLED");
    EvFilmGrainStrength = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_STRENGTH");
    EvFilmGrainSize = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_SIZE");
    EvFilmGrainRoughness = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_ROUGHNESS");

    const FilmGrainParams defaults;

    strength = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_STRENGTH"), 0., 100., 1., defaults.strength));
    size = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_SIZE"), 0.5, 6., 0.1, defaults.size));
    roughness = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_ROUGHNESS"), 0., 100., 1., defaults.roughness));
    size->set_tooltip_text(M("TP_FILMGRAIN_SIZE_TOOLTIP"));
    roughness->set_tooltip_text(M("TP_FILMGRAIN_ROUGHNESS_TOOLTIP"));

    for (Adjuster *adjuster : {strength, size, roughness}) {
        adjuster->setAdjusterListener(this);
        adjuster->show();
        pack_start(*adjuster);
    }
}


void FilmGrain::read(const ProcParams *pp, const ParamsEdited *pedited)
{
    disableListener();

    if (pedited) {
        strength->setEditedState(pedited->filmGrain.strength ? Edited : UnEdited);
        size->setEditedState(pedited->filmGrain.size ? Edited : UnEdited);
        roughness->setEditedState(pedited->filmGrain.roughness ? Edited : UnEdited);
        set_inconsistent(multiImage && !pedited->filmGrain.enabled);
    }

    setEnabled(pp->filmGrain.enabled);
    strength->setValue(pp->filmGrain.strength);
    size->setValue(pp->filmGrain.size);
    roughness->setValue(pp->filmGrain.roughness);

    enableListener();
}


void FilmGrain::write(ProcParams *pp, ParamsEdited *pedited)
{
    pp->filmGrain.strength = strength->getIntValue();
    pp->filmGrain.size = size->getValue();
    pp->filmGrain.roughness = roughness->getIntValue();
    pp->filmGrain.enabled = getEnabled();

    if (pedited) {
        pedited->filmGrain.strength = strength->getEditedState();
        pedited->filmGrain.size = size->getEditedState();
        pedited->filmGrain.roughness = roughness->getEditedState();
        pedited->filmGrain.enabled = !get_inconsistent();
    }
}

void FilmGrain::setDefaults(const ProcParams *defParams, const ParamsEdited *pedited)
{
    strength->setDefault(defParams->filmGrain.strength);
    size->setDefault(defParams->filmGrain.size);
    roughness->setDefault(defParams->filmGrain.roughness);

    if (pedited) {
        strength->setDefaultEditedState(pedited->filmGrain.strength ? Edited : UnEdited);
        size->setDefaultEditedState(pedited->filmGrain.size ? Edited : UnEdited);
        roughness->setDefaultEditedState(pedited->filmGrain.roughness ? Edited : UnEdited);
    } else {
        strength->setDefaultEditedState(Irrelevant);
        size->setDefaultEditedState(Irrelevant);
        roughness->setDefaultEditedState(Irrelevant);
    }
}


void FilmGrain::adjusterChanged(Adjuster* a, double newval)
{
    if (listener && getEnabled()) {
        if (a == strength) {
            listener->panelChanged(EvFilmGrainStrength, a->getTextValue());
        } else if (a == size) {
            listener->panelChanged(EvFilmGrainSize, a->getTextValue());
        } else if (a == roughness) {
            listener->panelChanged(EvFilmGrainRoughness, a->getTextValue());
        }
    }
}

void FilmGrain::enabledChanged ()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_DISABLED"));
        }
    }
}


void FilmGrain::setBatchMode(bool batchMode)
{
    ToolPanel::setBatchMode(batchMode);

    strength->showEditedCB();
    size->showEditedCB();
    roughness->showEditedCB();
}
