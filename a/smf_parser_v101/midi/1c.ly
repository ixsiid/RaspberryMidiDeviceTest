% *******************************************************
% ** FOLKLORE DEL LLUÇANÈS - JOSEP M. VILARMAU CABANES **
% ** ED. GRUP DE RECERCA FOLKLÒRICA D'OSONA *************
% ** (C) de la transcripció: Joan Quintana **************
% *******************************************************

\version "2.12.3"
\header {
	title = \markup {
         \override #'(font-name . "SpectrumMT SC")
			\fontsize #-3.5 
         "1. Don Lluís de Montalbà"
     } 
	composer = \markup {
         \override #'(font-name . "SpectrumMT SC")
			\fontsize #-5 
         "Isidre Collell Collell - Salselles"
     }
	tagline = "" %per tal d'eliminar el footer
}
#(set-default-paper-size "a6" 'landscape)


melodia =
\relative c''
{
  \set Staff.midiInstrument = #"fiddle"
  \clef treble
  \key a \major
  \time 2/4
  \tempo 4=83
r4 cis8 d e4. cis8 d4. b8 cis4 a~ a8 r gis a b4 b b cis b b8   \key c \major cis b4. a8 fis4 b a2~ a8 r cis d e4. cis8 d4. b8 cis4 a a8 r gis a b4 b b cis b b8 cis b4. a8 fis4 b a2~ a8 r r4
\bar "|."
}

\score {
  \melodia
  \layout { #(layout-set-staff-size 15) }
  \midi { }
}

