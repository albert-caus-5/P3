#!/bin/bash

# Establecemos que el código de retorno de un pipeline sea el del último programa con código de retorno
# distinto de cero, o cero si todos devuelven cero.
set -o pipefail

threshold_lag=${1:-0.39}
clipmult=${2:-0.0075}
threshold_r1r0=${3:-0.55}
threshold_zcr=${4:-30}
medfilt=${5:-1}
# Put here the program (maybe with path)
GETF0="get_pitch --threshold_lag=$threshold_lag --clipmult=$clipmult --threshold_r1r0=$threshold_r1r0 --threshold_zcr=$threshold_zcr --medfilt=$medfilt"

for fwav in pitch_db/train/*.wav; do
    ff0=${fwav/.wav/.f0}
    echo "$GETF0 $fwav $ff0 ----"
	$GETF0 $fwav $ff0 > /dev/null || ( echo -e "\nError in $GETF0 $fwav $ff0" && exit 1 )
done

pitch_evaluate pitch_db/train/*.f0ref

exit 0