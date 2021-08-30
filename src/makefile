#	Copyright 2019-2021 G. Adam Stanislav
#	All rights reserved
#
#	Redistribution and use in source and binary forms,
#	with or without modification, are permitted provided
#	that the following conditions are met:
#
#	1. Redistributions of source code must retain the
#	above copyright notice, this list of conditions
#	and the following disclaimer.
#
#	2. Redistributions in binary form must reproduce the
#	above copyright notice, this list of conditions and
#	the following disclaimer in the documentation and/or
#	other materials provided with the distribution.
#
#	3. Neither the name of the copyright holder nor the
#	names of its contributors may be used to endorse or
#	promote products derived from this software without
#	specific prior written permission.
#
#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
#	AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
#	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
#	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
#	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
#	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CC=gcc
CFLG=-O3 -s
CFLAGS=$(CFLG) -c -fPIC 
LDFLAGS=$(CFLG) --shared
# Change the next two lines as needed
K=koliba_
PREFIX=/usr/lib/frei0r-1

objects=601-2020.o \
	allnatcons.o \
	anachromatic.o \
	artdeco.o \
	bleachbypass.o \
	chanex.o \
	chanex2.o \
	chblend.o \
	chroma.o \
	chromatomorphosis.o \
	chrzone.o \
	colordials.o \
	colorroller.o \
	colors.o \
	crimsonite.o \
	diachromatic.o \
	dichromatic.o \
	erythropy.o \
	fromsrgb.o \
	gains.o \
	gcomp.o \
	index.o \
	invert.o \
	lifts.o \
	lut.o \
	lutty.o \
	monofarba.o \
	natcon.o \
	objective.o \
	orion-betelgeuse.o \
	palette.o \
	pinkfade.o \
	primary-secondary.o \
	purecolor.o \
	ranges.o \
	redmonofarba.o \
	saturation.o \
	selene-nyx.o \
	setfrange.o \
	solidcolor.o \
	tetramat.o \
	tint.o \
	tosrgb.o \
	vampyrectomy.o \
	vcchain.o \
	vcflutter.o \
	vcqube.o \
	vertsat.o \
	vivid.o \
	warm-and-cold.o \
	warm-cold.o 

libs=$(K)601-2020.so \
	$(K)allnatcons.so \
	$(K)anachromatic.so \
	$(K)artdeco.so \
	$(K)bleachbypass.so \
	$(K)chanex.so \
	$(K)chanex2.so \
	$(K)chblend.so \
	$(K)chroma.so \
	$(K)chromatomorphosis.so \
	$(K)chrzone.so \
	$(K)colordials.so \
	$(K)colorroller.so \
	$(K)colors.so \
	$(K)crimsonite.so \
	$(K)diachromatic.so \
	$(K)dichromatic.so \
	$(K)erythropy.so \
	$(K)fromsrgb.so \
	$(K)gains.so \
	$(K)gcomp.so \
	$(K)index.so \
	$(K)invert.so \
	$(K)lifts.so \
	$(K)lut.so \
	$(K)lutty.so \
	$(K)monofarba.so \
	$(K)natcon.so \
	$(K)objective.so \
	$(K)orion-betelgeuse.so \
	$(K)palette.so \
	$(K)pinkfade.so \
	$(K)primary-secondary.so \
	$(K)purecolor.so \
	$(K)ranges.so \
	$(K)redmonofarba.so \
	$(K)saturation.so \
	$(K)selene-nyx.so \
	$(K)setfrange.so \
	$(K)solidcolor.so \
	$(K)tetramat.so \
	$(K)tint.so \
	$(K)tosrgb.so \
	$(K)vampyrectomy.so \
	$(K)vcchain.so \
	$(K)vcflutter.so \
	$(K)vcqube.so \
	$(K)vertsat.so \
	$(K)vivid.so \
	$(K)warm-and-cold.so \
	$(K)warm-cold.so 

installs=install-601-2020 \
	install-allnatcons \
	install-anachromatic \
	install-artdeco \
	install-bleachbypass \
	install-chanex \
	install-chanex2 \
	install-chblend \
	install-chroma \
	install-chromatomorphosis \
	install-chrzone \
	install-colordials \
	install-colorroller \
	install-colors \
	install-crimsonite \
	install-diachromatic \
	install-dichromatic \
	install-erythropy \
	install-fromsrgb \
	install-gains \
	install-gcomp \
	install-index \
	install-invert \
	install-lifts \
	install-lut \
	install-lutty \
	install-monofarba \
	install-natcon \
	install-objective \
	install-orion-betelgeuse \
	install-palette \
	install-pinkfade \
	install-primary-secondary \
	install-purecolor \
	install-ranges \
	install-redmonofarba \
	install-saturation \
	install-selene-nyx \
	install-setfrange \
	install-solidcolor \
	install-tetramat \
	install-tint \
	install-tosrgb \
	install-vampyrectomy \
	install-vcchain \
	install-vcflutter \
	install-vcqube \
	install-vertsat \
	install-vivid \
	install-warm-and-cold \
	install-warm-cold 


all: $(libs)

install: all $(installs)

$(K)601-2020.so: 601-2020.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)allnatcons.so: allnatcons.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)anachromatic.so: anachromatic.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)artdeco.so: artdeco.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)bleachbypass.so: bleachbypass.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chanex.so: chanex.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chanex2.so: chanex2.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chblend.so: chblend.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chroma.so: chroma.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chromatomorphosis.so: chromatomorphosis.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)chrzone.so: chrzone.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)colordials.so: colordials.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)colorroller.so: colorroller.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)colors.so: colors.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)crimsonite.so: crimsonite.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)diachromatic.so: diachromatic.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)dichromatic.so: dichromatic.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)erythropy.so: erythropy.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)fromsrgb.so: fromsrgb.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)gains.so: gains.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)gcomp.so: gcomp.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)index.so: index.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)invert.so: invert.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)lifts.so: lifts.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)lut.so: lut.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)lutty.so: lutty.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)monofarba.so: monofarba.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)natcon.so: natcon.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)objective.so: objective.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)orion-betelgeuse.so: orion-betelgeuse.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)palette.so: palette.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)pinkfade.so: pinkfade.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)primary-secondary.so: primary-secondary.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)purecolor.so: purecolor.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)ranges.so: ranges.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)redmonofarba.so: redmonofarba.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)saturation.so: saturation.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)selene-nyx.so: selene-nyx.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)setfrange.so: setfrange.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)solidcolor.so: solidcolor.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)tetramat.so: tetramat.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)tint.so: tint.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)tosrgb.so: tosrgb.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vampyrectomy.so: vampyrectomy.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vcchain.so: vcchain.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vcflutter.so: vcflutter.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vcqube.so: vcqube.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vertsat.so: vertsat.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)vivid.so: vivid.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)warm-and-cold.so: warm-and-cold.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

$(K)warm-cold.so: warm-cold.o
	$(CC) $(LDFLAGS) $< -o $@ -lkoliba

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.so: %.o
	$(CC) $(LDFLAGS) $< -o $@ -L. -lkoliba

install-601-2020: $(K)601-2020.so
	install -p -s $? $(PREFIX)
	touch install-601-2020

install-allnatcons: $(K)allnatcons.so
	install -p -s $? $(PREFIX)
	touch install-allnatcons

install-anachromatic: $(K)anachromatic.so
	install -p -s $? $(PREFIX)
	touch install-anachromatic

install-artdeco: $(K)artdeco.so
	install -p -s $? $(PREFIX)
	touch install-artdeco

install-bleachbypass: $(K)bleachbypass.so
	install -p -s $? $(PREFIX)
	touch install-bleachbypass

install-chanex: $(K)chanex.so
	install -p -s $? $(PREFIX)
	touch install-chanex

install-chanex2: $(K)chanex2.so
	install -p -s $? $(PREFIX)
	touch install-chanex2

install-chblend: $(K)chblend.so
	install -p -s $? $(PREFIX)
	touch install-chblend

install-chroma: $(K)chroma.so
	install -p -s $? $(PREFIX)
	touch install-chroma

install-chromatomorphosis: $(K)chromatomorphosis.so
	install -p -s $? $(PREFIX)
	touch install-chromatomorphosis

install-chrzone: $(K)chrzone.so
	install -p -s $? $(PREFIX)
	touch install-chrzone

install-colordials: $(K)colordials.so
	install -p -s $? $(PREFIX)
	touch install-colordials

install-colorroller: $(K)colorroller.so
	install -p -s $? $(PREFIX)
	touch install-colorroller

install-colors: $(K)colors.so
	install -p -s $? $(PREFIX)
	touch install-colors

install-crimsonite: $(K)crimsonite.so
	install -p -s $? $(PREFIX)
	touch install-crimsonite

install-diachromatic: $(K)diachromatic.so
	install -p -s $? $(PREFIX)
	touch install-diachromatic

install-dichromatic: $(K)dichromatic.so
	install -p -s $? $(PREFIX)
	touch install-dichromatic

install-erythropy: $(K)erythropy.so
	install -p -s $? $(PREFIX)
	touch install-erythropy

install-fromsrgb: $(K)fromsrgb.so
	install -p -s $? $(PREFIX)
	touch install-fromsrgb

install-gains: $(K)gains.so
	install -p -s $? $(PREFIX)
	touch install-gains

install-gcomp: $(K)gcomp.so
	install -p -s $? $(PREFIX)
	touch install-gcomp

install-index: $(K)index.so
	install -p -s $? $(PREFIX)
	touch install-index

install-invert: $(K)invert.so
	install -p -s $? $(PREFIX)
	touch install-invert

install-lifts: $(K)lifts.so
	install -p -s $? $(PREFIX)
	touch install-lifts

install-lut: $(K)lut.so
	install -p -s $? $(PREFIX)
	touch install-lut

install-lutty: $(K)lutty.so
	install -p -s $? $(PREFIX)
	touch install-lutty

install-monofarba: $(K)monofarba.so
	install -p -s $? $(PREFIX)
	touch install-monofarba

install-natcon: $(K)natcon.so
	install -p -s $? $(PREFIX)
	touch install-natcon

install-objective: $(K)objective.so
	install -p -s $? $(PREFIX)
	touch install-objective

install-orion-betelgeuse: $(K)orion-betelgeuse.so
	install -p -s $? $(PREFIX)
	touch install-orion-betelgeuse

install-palette: $(K)palette.so
	install -p -s $? $(PREFIX)
	touch install-palette

install-pinkfade: $(K)pinkfade.so
	install -p -s $? $(PREFIX)
	touch install-pinkfade

install-primary-secondary: $(K)primary-secondary.so
	install -p -s $? $(PREFIX)
	touch install-primary-secondary

install-purecolor: $(K)purecolor.so
	install -p -s $? $(PREFIX)
	touch install-purecolor

install-ranges: $(K)ranges.so
	install -p -s $? $(PREFIX)
	touch install-ranges

install-redmonofarba: $(K)redmonofarba.so
	install -p -s $? $(PREFIX)
	touch install-redmonofarba

install-saturation: $(K)saturation.so
	install -p -s $? $(PREFIX)
	touch install-saturation

install-selene-nyx: $(K)selene-nyx.so
	install -p -s $? $(PREFIX)
	touch install-selene-nyx

install-setfrange: $(K)setfrange.so
	install -p -s $? $(PREFIX)
	touch install-setfrange

install-solidcolor: $(K)solidcolor.so
	install -p -s $? $(PREFIX)
	touch install-solidcolor

install-tetramat: $(K)tetramat.so
	install -p -s $? $(PREFIX)
	touch install-tetramat

install-tint: $(K)tint.so
	install -p -s $? $(PREFIX)
	touch install-tint

install-tosrgb: $(K)tosrgb.so
	install -p -s $? $(PREFIX)
	touch install-tosrgb

install-vampyrectomy: $(K)vampyrectomy.so
	install -p -s $? $(PREFIX)
	touch install-vampyrectomy

install-vcchain: $(K)vcchain.so
	install -p -s $? $(PREFIX)
	touch install-vcchain

install-vcflutter: $(K)vcflutter.so
	install -p -s $? $(PREFIX)
	touch install-vcflutter

install-vcqube: $(K)vcqube.so
	install -p -s $? $(PREFIX)
	touch install-vcqube

install-vertsat: $(K)vertsat.so
	install -p -s $? $(PREFIX)
	touch install-vertsat

install-vivid: $(K)vivid.so
	install -p -s $? $(PREFIX)
	touch install-vivid

install-warm-and-cold: $(K)warm-and-cold.so
	install -p -s $? $(PREFIX)
	touch install-warm-and-cold

install-warm-cold: $(K)warm-cold.so
	install -p -s $? $(PREFIX)
	touch install-warm-cold

clean:
	rm -f $(libs) $(objects) $(installs)

