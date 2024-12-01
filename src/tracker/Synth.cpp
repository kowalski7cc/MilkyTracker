/*
 *  tracker/Synth.h 
 *
 *  Copyright 2023 Leon van Kammen (coderofsalvation)
 * 
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Synth.h"
#include <math.h>
#include "SampleEditor.h"
#include "DialogSliders.h"
#include "TrackerConfig.h"
#include "FilterParameters.h"

Synth::Synth(int samplerate){
  this->samplerate = samplerate;
  this->additive   = false;
  init();
  // assign default synth 
  synth = &synths[0];
}

Synth::~Synth(){
}
	
PPString Synth::ASCIISynthExport(  ) {       // see 
  PPString ASCIISynth = PPString(SYN_PREFIX_V1);
  for( int i = 0; i < SYN_PARAMS_MAX ; i++ ){ 
    ASCIISynth.append( i < synth->nparams ? (int)synth->param[i].value + SYN_OFFSET_CHAR : SYN_OFFSET_CHAR ); 
  }
  printf("ASCIISYNTH PRESET: '%s'\n",ASCIISynth.getStrBuffer());
  return ASCIISynth;
}

bool Synth::ASCIISynthImport( PPString preset ) {
  if( preset.startsWith(SYN_PREFIX_V1) ){ // detect synth version(s) 
    const char *str = preset.getStrBuffer();
    printf("ASCIISYNTH IMPORT: '%s'\n",str);
    int ID = str[ SYN_PREFIX_CHARS ] - SYN_OFFSET_CHAR; 
    synth = &(synths[ID]);
    for( int i = 0; i < preset.length() && i < SYN_PARAMS_MAX; i++ ){ 
       setParam(i, str[ i + SYN_PREFIX_CHARS ] - SYN_OFFSET_CHAR ); 
    }
    return true;
  }else return false;
}

void Synth::reset(){
  for( int i = 0; i < SYN_PARAMS_MAX; i++){ 
    synth->param[i].value = 0.0f;
    synth->param[i].name = PPString("");
  }
}

DialogSliders * Synth::dialog( SampleEditor *s, PPScreen *screen, DialogResponder *dr ){
  if( s != NULL && screen != NULL && dr != NULL ){
    this->sampleEditor = s;
    this->screen = screen;
    this->dr = dr;
  }else{
    sliders->show(false);
  }
  PPString title = PPString("milkysynth");
  this->additive = s != NULL ? s->hasValidSelection() : false;
  if( this->additive ) title.append(" [additive]");
  sliders = new DialogSliders( this->screen, this->dr, PP_DEFAULT_ID, title, synth->nparams, this->sampleEditor, &SampleEditor::tool_synth );
  sliders->show();
  for( int i = 0; i < synth->nparams && i < SYN_PARAMS_MAX; i++){
	PPString label = PPString(synth->param[i].name);
    PPFont *font   = NULL;
	PPColor *color = NULL; 
	if( i == 0 ) color = (PPColor *)&TrackerConfig::colorPatternEditorNote;
	if( label.length() > 14 ){
		font  = PPFont::getFont( label.length() > 14 ? PPFont::FONT_TINY : PPFont::FONT_SYSTEM );
		color = (PPColor *)&TrackerConfig::colorPatternEditorEffect;
	}
    sliders->initSlider(i, (int)synth->param[i].min, (int)synth->param[i].max, synth->param[i].value, label, color, font );
  }
  return sliders;
}

void Synth::setParam( int i, float v ){
  synth->param[i].value = v;
} 

void Synth::random(){
	pp_uint32 pr = rand() % SYNTH_PRESETS;
	ASCIISynthImport( preset[pr] );
    FilterParameters par(synth->nparams);
    pp_int32 i;
    for( i = 0; i < synth->nparams; i++ ){
      par.setParameter(i, FilterParameters::Parameter( synth->param[i].value ) );
    }
	if( !sampleEditor->isEmptySample() ){
		sampleEditor->clearSample();
	}
	sampleEditor->tool_synth(&par);
}

TXMSample * Synth::prepareSample( pp_uint32 duration){
  TXMSample *sample;
  FilterParameters par(2);
  par.setParameter(0, FilterParameters::Parameter( (pp_int32)duration ) );
  par.setParameter(1, FilterParameters::Parameter( 16 ) );
  sampleEditor->tool_newSample(&par);
  sample = sampleEditor->getSample();
  return sample;
}