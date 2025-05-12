/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -a REAL, --threshold_lag=REAL  Umbral de l'autocorrelación normalizada.[default: 0.39]
    -c REAL, --center_clipping=REAL  Valor del center clipping com a preprocessat. [default: 0.0075]
    -r REAL, --threshold_r1r0=REAL  Umbral de l'autocorrelació d'1 r[1]/r[0]. [default: 0.55]
    -z REAL, --threshold_zcr=REAL  Umbral del ZCR. [default: 30]
    -m REAL, --median_filter=REAL  Longitud del filtre de mitjanes com a postprocessat. [default: 1]

    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";
float abs_f(float value){
  if (value < 0.0)
    return -1.0*value;
  return value;
}
int main(int argc, const char *argv[]) {
	/// \TODO 
  /// \FET Crida a Docopt per facilitar l'execució
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string
 
	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float threshold_lag = stof(args["--threshold_lag"].asString());
  float center_clipping = stof(args["--center_clipping"].asString());
  float threshold_r1r0 = stof(args["--threshold_r1r0"].asString());
  float threshold_zcr = stof(args["--threshold_zcr"].asString());
  float median_filter = stof(args["--median_filter"].asString());

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 500, threshold_lag, threshold_r1r0, threshold_zcr);

  /// \TODO
  /// \FET Métode de preprocessat Center-Clipping
  /// Preprocess the input signal in order to ease pitch estimation. For instance, 
  /// central-clipping or low pass filtering may be used.

  std::vector<float>::iterator iX, it;
  vector<float> f0;
  // Iterate for each frame and save values in f0 vector

  // Agafem el valor màxim, tant si és positiu com negatiu
  float cent_clip = -1.0;
  for(iX = x.begin(); iX < x.end(); ++iX){    
    cent_clip = std::max(cent_clip, abs_f(*iX));
  }
  cent_clip = center_clipping * cent_clip;


  float f, aux_1, aux_2, aux_3, zcr=0; 
  float cte = rate / (2 * (n_len - 1));
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    aux_2=0; aux_1=0;
    //Combinació amb el ZCR
    for(it = iX; it < iX + n_len; ++it){ 
      aux_3 = *it;
      if((aux_3 * aux_2) < 0){ aux_1++;}
      aux_2 = aux_3;

      if(abs(aux_3) < cent_clip){ *it = 0;}
      else *it = *it + cent_clip * ((aux_3 < 0) - (aux_3 > 0));
    }
    zcr = aux_1 * cte;

    f = analyzer(iX, iX + n_len, zcr);
    f0.push_back(f);
  }


  
  /// \TODO
  /// \FET Métode de postprocessat media filter
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  int size = median_filter;  
  vector<float> filter; 

  for(iX = f0.begin(); iX < f0.end() - (size - 1); ++iX){    
    for(int i = 0; i<size; i++)      
      filter.push_back(*(iX+i)); 
    
    int i, j;
    for(i = 0; i < size-1; i++){ 
      for(j = 0; j < size-i-1; j++){
        if (filter[j] > filter[j+1]){        
          aux_1 = filter[j];        
          filter[j] = filter[j+1]; 
          filter[j+1] = aux_1;      
        }
      }
    }
    f0[iX - f0.begin()] = filter[size/2];
    filter.clear();  
  } 

  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}