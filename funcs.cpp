#include "funcs.hpp"

int largest_contour_index(std::vector<std::vector<cv::Point>> vec) {
    int area = 0, index = 0; // Returns 0 if null
    for(int i = 0; i < vec.size(); i++) {
        int comparator = cv::contourArea(vec[i]);
        if(area < comparator) {
            area = comparator;
            index = i;
        }
    }
    return index;
}

void find_and_store_peaks(std::vector<cv::Point2f> &positions, std::vector<cv::Point2f> &amplitudes) {
    if (positions.size() > 2) {
    int n = positions.size();
    float prev = positions[n - 3].y;
    float curr = positions[n - 2].y;
    float next = positions[n - 1].y;

    // Peak detection (local max or min)
    // This is exactly one of the exercicises we did in Fund. Prog!
    if ((curr >= prev && curr >= next) || (curr <= prev && curr <= next)) {
            amplitudes.push_back(cv::Point2f(positions[n - 2].x, curr));
        }
    }
}

void filter_amplitudes(std::vector<cv::Point2f> &amplitudes) {

    std::vector<cv::Point2f> filtered;

    filtered.push_back(amplitudes[0]);

    for (int i = 0; i < amplitudes.size(); i++) {
        float dt = amplitudes[i].x - filtered.back().x;

        // Reject if peaks are too close in time or if displacement is too large
        if (dt < 0.2f) continue;           

        filtered.push_back(amplitudes[i]);
    }

    amplitudes = filtered;
}

Pendulum calculate_variables(std::vector<cv::Point2f> &amplitudes, float l, float pxpm, double y_eq_px) {
    Pendulum pend = {0};

    if (amplitudes.size() < 3) {
        std::cerr << "[Aviso] Dados insuficientes. São necessários pelo menos 3 picos.\n";
        return pend;
    }

    pend.init_pos = y_eq_px / pxpm;
    pend.A = (std::abs(amplitudes[0].y - y_eq_px) / pxpm) * 10; // meters

    double total_period = 0.0;
    double total_damping_b = 0.0;
    int valid_cycles = 0;

    // Use same-sign peaks separated by one full oscillation (i and i+2)
    for (size_t i = 0; i + 2 < amplitudes.size(); ++i) {
        const auto& p1 = amplitudes[i];
        const auto& p3 = amplitudes[i + 2];

        double dt = static_cast<double>(p3.x - p1.x); // time between same-side peaks
        double amp1 = std::abs(p1.y - y_eq_px) / pxpm; // meters
        double amp3 = std::abs(p3.y - y_eq_px) / pxpm; // meters

        // Basic validity checks
        if (dt < 0.01) continue;        // too short
        if (amp1 <= 0.0 || amp3 <= 0.0) continue;

        // Period contribution: dt is one full period
        total_period += dt;

        // damping b = ln(A1/A3) / dt  (A(t) = A0 * exp(-b t))
        double b_local = 0.0;
        if (amp3 > 0.0 && amp1 > amp3) {
            b_local = std::log(amp1 / amp3) / dt;
            if (std::isfinite(b_local) && b_local >= 0.0) {
                total_damping_b += b_local;
            } else {
                continue; // skip non-finite or negative values
            }
        } else {
            continue;
        }

        valid_cycles++;
    }

    if (valid_cycles == 0) {
        std::cerr << "[Aviso] Nenhum ciclo válido detectado (verifique picos, ruído e ordenação das amostras).\n";
        return pend;
    }

    pend.T_exp = total_period / valid_cycles;
    pend.b = total_damping_b / valid_cycles;
    pend.omega = (2.0 * M_PI) / pend.T_exp;
    if (pend.b > 0.0) pend.quality_factor = pend.omega / (2.0 * pend.b);

    if (l > 0.0) {
        const double g = 9.81; // or 10.0 if you prefer your g=10 convention
        pend.T_teo = (2.0 * M_PI) * std::sqrt(l / g);
        if (pend.T_teo > 0.0) pend.erro = std::abs(pend.T_exp - pend.T_teo) / pend.T_teo * 100.0;
    }

    return pend;
}



void print_pendulum(Pendulum pend) {
    std::cout << "\n=== Pendulum Parameters ===\n";
    std::cout << "Amplitude (A)       : " << pend.A << " m\n";
    std::cout << "Angular frequency ω : " << pend.omega << " rad/s\n";
    std::cout << "Damping coefficient : " << pend.b << " 1/s\n";
    std::cout << "Quality factor (Q)  : " << pend.quality_factor << "\n";
    std::cout << "Período experimental: " << pend.T_exp << " s\n";
    std::cout << "Período teórico     : " << pend.T_teo << " s\n";
    std::cout << "Erro relativo       : " << pend.erro << " %\n";
    std::cout << "============================\n";
}
