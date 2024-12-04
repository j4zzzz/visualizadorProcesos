#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <unistd.h> 
#include <iterator> 
#include <chrono>

class Proceso {
public:
    std::string nombre;
    int id;
    int rafaga;
    int tiempoLlegada;
    char estado;

    Proceso(const std::string& nombre, int id, int rafaga, int tiempoLlegada, char estado = 'N')
        : nombre(nombre), id(id), rafaga(rafaga), tiempoLlegada(tiempoLlegada), estado(estado) {}
};

class CPU {
public:
    std::vector<Proceso> procesos;

    void agregarProceso(const std::string& nombre, int id, int rafaga, int tiempoLlegada) {
        procesos.emplace_back(nombre, id, rafaga, tiempoLlegada);
    }
};

std::string convertirTicksAHora(long long startTimeTicks) {
    long ticksPorSegundo = sysconf(_SC_CLK_TCK);
    long long tiempoInicioSegundos = startTimeTicks / ticksPorSegundo;

    auto ahora = std::chrono::system_clock::now();
    auto tiempoEpoch = std::chrono::duration_cast<std::chrono::seconds>(ahora.time_since_epoch()).count();

    std::ifstream uptimeFile("/proc/uptime");
    double tiempoEncendido;
    if (uptimeFile.is_open()) {
        uptimeFile >> tiempoEncendido;
    }

    long long tiempoInicioEpoch = tiempoEpoch - static_cast<long long>(tiempoEncendido) + tiempoInicioSegundos;
    std::time_t tiempoInicio = static_cast<std::time_t>(tiempoInicioEpoch);
    std::tm* tm = std::localtime(&tiempoInicio);

    std::ostringstream oss;
    oss << std::put_time(tm, "%H:%M:%S");
    return oss.str();
}

void capturarProcesos(CPU& cpu) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 10);


    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();

            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
                try {
                    std::ifstream commFile(entry.path() / "comm");
                    std::string nombreProceso;
                    if (commFile.is_open() && std::getline(commFile, nombreProceso)) {
                        int pid = std::stoi(dirName);
                        int rafaga = distrib(gen);

                        std::ifstream statFile(entry.path() / "stat");
                        std::string statLine;
                        if (statFile.is_open() && std::getline(statFile, statLine)) {
                            std::istringstream iss(statLine);
                            std::vector<std::string> statTokens{
                                std::istream_iterator<std::string>(iss),
                                std::istream_iterator<std::string>()
                            };

                            if (statTokens.size() > 21) {
                                try {
                                    long long startTimeTicks = std::stoll(statTokens[21]);
                                    int tiempoLlegada = 0;

                                    if (tiempoReferencia == -1) {
                                        tiempoReferencia = startTimeTicks / sysconf(_SC_CLK_TCK);
                                    } else {
                                        int tiempoEnSegundos = startTimeTicks / sysconf(_SC_CLK_TCK);
                                        tiempoLlegada = tiempoEnSegundos - tiempoReferencia;
                                    }

                                    cpu.agregarProceso(nombreProceso, pid, rafaga, tiempoLlegada);
                                } catch (const std::invalid_argument& e) {
                                    std::cerr << "Error al convertir el tiempo de inicio para el PID " << pid << std::endl;
                                }
                            } else {
                                std::cerr << "El proceso con PID " << pid << " no tiene suficientes datos en /proc/[PID]/stat." << std::endl;
                            }
                        }
                    }
                } catch (...) {
                }
            }
        }
    }
}

int main() {
    CPU cpu;

    capturarProcesos(cpu);

    for (const auto& proceso : cpu.procesos) {
        std::cout << "Nombre: " << proceso.nombre
                  << ", PID: " << proceso.id
                  << ", Ráfaga: " << proceso.rafaga
                  << ", Hora de Llegada: " << proceso.tiempoLlegada
                  << ", Estado: " << proceso.estado
                  << std::endl;
    }

    return 0;
}

