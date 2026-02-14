#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <vector>   // Dodano dla obsługi listy wyników
#include <numeric>  // Dodano dla obliczania średniej

using namespace std;

int main()
{

    int range = 650000;  // liczba próbek do wczytania
    string temp;
    double maxi, mini;
    //double samples[range], fil[range], fild[range], squared[range];
    vector<double> samples(range);
    vector<double> fil(range);
    vector<double> fild(range);
    vector<double> squared(range);

    ifstream reader("samples_203.csv");

    if (!reader.is_open()) {
        cout << "Blad: Nie mozna otworzyc pliku samples_203.csv" << endl;
        return 1;
    }

    getline(reader, temp);  // Ignorowanie nagłówka
    getline(reader, temp);  // Ignorowanie kolejnej linii nagłówka

    // Wczytanie danych z pliku do tablicy samples
    for (int i = 0; i < range && reader.good(); i++) {
        getline(reader, temp, ',');
        getline(reader, temp);
        try {
            samples[i] = stof(temp) * 200;  // Przemnożenie próbek przez 200
        } catch (...) {
            samples[i] = 0;
        }
    }

    // Obliczanie max i min w samples
    maxi = mini = samples[0];
    for (int i = 0; i < range; i++) {
        maxi = max(maxi, samples[i]);
        mini = min(mini, samples[i]);
    }
    float zakres = fabs(maxi - mini);

    // Filtracja próbek: subtract previous value
    for (int i = 7; i < range; i++) {
        fil[i] = samples[i] - samples[i - 7];
    }
    // Uzupełnienie zerami początkowych próbek dla bezpieczeństwa
    for (int i = 0; i < 7; i++) fil[i] = 0;

    // Obliczanie max i min w fil
    double maxf = fil[7], minf = fil[7];
    for (int i = 7; i < range; i++) {
        maxf = max(maxf, fil[i]);
        minf = min(minf, fil[i]);
    }
    float zakresf = fabs(maxf - minf);

    // Uśrednianie próbek
    for (int i = 7; i < range; i++) {
        fild[i] = 0;
        for (int k = 0; k < 8; k++) {
            fild[i] += fil[i - k];
        }
        fild[i] /= 8;
    }
    for (int i = 0; i < 7; i++) fild[i] = 0;

    // Obliczanie max i min w fild
    double maxd = fild[7], mind = fild[7];
    for (int i = 7; i < range; i++) {
        maxd = max(maxd, fild[i]);
        mind = min(mind, fild[i]);
    }
    float zakresd = fabs(maxd - mind);

    // Kwadratowanie próbek
    for (int i = 0; i < range; i++) {
        squared[i] = fild[i] * fild[i];
    }

    // Obliczanie max i min w squared
    double maxsq = squared[0], minsq = squared[0];
    for (int i = 0; i < range; i++) {
        maxsq = max(maxsq, squared[i]);
        minsq = min(minsq, squared[i]);
    }
    float zakressq = fabs(maxsq - minsq);

    // Szerokość wykresu
    int szerokosc = 160;
    int zero = szerokosc / 2;


    // Rysowanie wykresu (zostawiamy tak jak było)
    for (int i = 0; i < range; i++) {
        int offset = (zakres > 0) ? static_cast<int>((samples[i] / zakres) * (szerokosc / 2)) : 0;
        int offsetf = (zakresf > 0) ? static_cast<int>((fil[i] / zakresf) * (szerokosc / 2)) : 0;
        int offsetd = (zakresd > 0) ? static_cast<int>((fild[i] / zakresd) * (szerokosc / 2)) : 0;
        int offsetsq = (zakressq > 0) ? static_cast<int>((squared[i] / zakressq) * (szerokosc / 2)) : 0;
/*
        for (int j = 0; j < szerokosc; j++) {
            if (j == zero)
                cout << "|";
            else if (j == (zero + offset))
                cout << "*";
            else if (j == (zero + offsetf))
                cout << "&";
            else if (j == (zero + offsetd))
                cout << "$";
            else if (j == (zero + offsetsq))
                cout << "^";
            else
                cout << " ";
        }

        cout << endl;
    }
*/
    }
    cout << "\n--------------------------------------------------\n";
    cout << "WYNIKI DETEKTORA QRS" << endl;
    cout << "--------------------------------------------------\n";

    // Zmienne stanu i parametry ze zdjęci
    int state = 3; 
    double threshold = maxsq * 0.3; // Początkowy próg (bezpieczna wartość startowa, np. 30% max)

    // Wektory do przechowywania historii
    vector<int> qrs_peaks_indices;      // Wynik: indeksy wykrytych załamków
    vector<double> detected_max_values; // Historia wartości maksymalnych do średniej

    // Zmienne pomocnicze dla maszyny stanów
    int state_timer = 0;       // Licznik czasu w danym stanie
    double local_max = 0;      // Lokalny max w Stanie 1
    int local_max_idx = 0;     // Indeks lokalnego max

    // Pętla po wszystkich próbkach sygnału 'squared' (y[n])
    for (int n = 0; n < range; n++) {
        double yn = squared[n]; // Wartość sygnału y[n]

        if (state == 3) {
            // Wzór: th[n] = th[n-1] * 0.98165

            if (yn > threshold) {
                // Warunek y[n] > th[n] spełniony -> Przejście do Stanu 1
                state = 1;
                state_timer = 0;
                local_max = yn;
                local_max_idx = n;
            } else {
                // Obniżanie progu
                threshold = threshold * 0.98165;
            }
        }
        else if (state == 1) {
            // Czas trwania: 94 próbki

            if (yn > local_max) {
                local_max = yn;
                local_max_idx = n;
            }

            state_timer++;

            if (state_timer >= 94) {
                // Koniec czasu poszukiwania (94 próbki minęły)
                // 1. Zapisz znaleziony QRS
                qrs_peaks_indices.push_back(local_max_idx);
                detected_max_values.push_back(local_max);

                // 2. Oblicz nowy próg startowy (średnia z poprzednio znalezionych wartości)
                double avg_max = 0;
                for (double v : detected_max_values) avg_max += v;
                avg_max /= detected_max_values.size();
                threshold = avg_max; // Ustawienie nowego progu startowego

                // 3. Przejście do Stanu 2
                state = 2;
            }
        }
        else if (state == 2) {
            // Warunek: 72 próbki od wartości maksymalnej (local_max_idx)

            int samples_since_peak = n - local_max_idx;

            if (samples_since_peak >= 72) {
                state = 3;
            }
        }
    }

    //WYJŚCIE

    cout << "Wykryte indeksy zespolow QRS:" << endl;
    cout << "[ ";
    for (size_t i = 0; i < qrs_peaks_indices.size(); i++) {
        cout << qrs_peaks_indices[i];
        if (i < qrs_peaks_indices.size() - 1) cout << ", ";
    }
    cout << " ]" << endl;

    cout << "Liczba wykrytych zespolow: " << qrs_peaks_indices.size() << endl;

    reader.close();
    
    
   // --------------------------------------------------------------
//Analiza trafności wykrywania QRS z pliku
// --------------------------------------------------------------

{
    ifstream anomalies_file("annotations_203m.txt");
    if (!anomalies_file.is_open()) {
        cout << "COOOOOOOOOOOOOOOOOOOOO" << endl;
    } else {

        vector<int> true_anomalies;
        string line;

        while (getline(anomalies_file, line)) {

            if (line.empty()) continue;

            size_t comma_pos = line.find(',');
            if (comma_pos == string::npos) continue;   // brak przecinka → pomiń

            string number_part = line.substr(0, comma_pos);
            string label_part  = line.substr(comma_pos + 1);

            // Usunięcie spacji
            while (!label_part.empty() && isspace(label_part.back()))
                label_part.pop_back();

            while (!label_part.empty() && isspace(label_part.front()))
                label_part.erase(label_part.begin());

            // Jeśli etykieta NIE jest pojedynczą dużą literą A-Z → POMIŃ
            if (label_part.size() != 1 || !(label_part[0] >= 'A' && label_part[0] <= 'Z')) {
                continue;
            }

            // Wczytaj indeks
            try {
                int idx = stoi(number_part);
                true_anomalies.push_back(idx);
            } catch (...) {
                continue;
            }
        }

        anomalies_file.close();
        cout << "ANALIZA TRAFNOSCI DETEKCJI (zakres ±54 probek)\n";

        int hits = 0;

        for (int real_idx : true_anomalies) {

            bool hit = false;

            for (int detected : qrs_peaks_indices) {
                if (abs(detected - real_idx) <= 54) {
                    hit = true;
                    break;
                }
            }

            cout << "Prawdziwy QRS: " << real_idx
                 << "  -->  " << (hit ? "TRAFIONO" : "Nie tym razem gagatku") << endl;

            if (hit) hits++;
        }

        double accuracy = true_anomalies.empty() ? 0.0
                       : (double)hits / true_anomalies.size();

        cout << "----------------------------------------------\n";
        cout << "Teoretyczna Liczba anomalii wyczytanych: 2980"<< endl;
        cout << "Liczba prawdziwych anomalii: " << true_anomalies.size() << endl;
        cout << "liczba false positive: "<< true_anomalies.size() - hits << endl;
        cout << "Liczba trafionych: " << hits << endl;
        cout << "Wspolczynnik trafnosci: " << accuracy * 100.0 << " %" << endl;
        cout << "----------------------------------------------\n";
    }
}
    return 0;
}
