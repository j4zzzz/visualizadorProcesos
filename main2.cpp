#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>
#include <unistd.h> 
#include <iterator> 
#include <algorithm> 

// Clase Proceso
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
// Clase CPU
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

    int tiempoReferencia = -1;  

    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();

            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
                try {
                    std::ifstream commFile(entry.path() / "comm");
                    std::string nombreProceso;
                    if (commFile.is_open() && std::getline(commFile, nombreProceso)) {
                        int pid = std::stoi(dirName);
                        //std::cout<<pid<<" ";
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



// Función FIFO
int fifo(CPU& cpu) {
    if (!cpu.procesos.empty()) {
        cpu.procesos[0].rafaga -= 1; 
        if (cpu.procesos[0].rafaga <= 0) {
            cpu.procesos.erase(cpu.procesos.begin()); 
            for (size_t i = 0; i < cpu.procesos.size(); ++i) {
                cpu.procesos[i].id = i; 
            }
        }
        return 0;
    }
    return -1; // si no hay procesos
}

int rr(CPU& cpu, int pos) {
    if (pos >= 0 && pos < static_cast<int>(cpu.procesos.size())) {
        cpu.procesos[pos].rafaga -= 1; 
        if (cpu.procesos[pos].rafaga <= 0) {
            cpu.procesos.erase(cpu.procesos.begin() + pos);
            
            return (pos >= static_cast<int>(cpu.procesos.size())) ? 0 : pos; 
        }
        return pos; 
    }
    return -1; 

} 
int sjf(CPU& cpu) {
    if (!cpu.procesos.empty()) {
        std::sort(cpu.procesos.begin(), cpu.procesos.end(), [](const Proceso& a, const Proceso& b) {
            return a.rafaga < b.rafaga;
        });

        cpu.procesos[0].rafaga -= 1; 
        if (cpu.procesos[0].rafaga == 0) {
            cpu.procesos.erase(cpu.procesos.begin()); 
        }
        return 0; 
    }
    return -1;
}
void dibujarCPU(sf::RenderWindow& ventana, const CPU& cpu, int inicioY, const std::string& titulo, int procesoActual) {
    sf::Font fuente;
    if (!fuente.loadFromFile("fonts/Arial.ttf")) {
        std::cerr << "No se pudo cargar la fuente." << std::endl;
        return;
    }

    // nombre del proceso actual
    sf::Text textoTitulo;
    textoTitulo.setFont(fuente);
    /*
    if (procesoActual >= 0 && procesoActual < static_cast<int>(cpu.procesos.size())) {
        textoTitulo.setString(titulo + " - Proceso actual: " + cpu.procesos[procesoActual].nombre);
    } else {
        textoTitulo.setString(titulo + " - Proceso actual: Ninguno");
    }
    textoTitulo.setCharacterSize(24);
    */
    textoTitulo.setFillColor(sf::Color::White);
    textoTitulo.setPosition(10, inicioY + 10);
    ventana.draw(textoTitulo);

    int x = 10;
    int y = inicioY + 200;
    for (size_t i = 0; i < 40; ++i) {
        if (i < cpu.procesos.size()) {
            sf::Color color = (i == static_cast<size_t>(procesoActual)) ? sf::Color::Yellow : sf::Color::Blue;

            // Dibujar barra
            sf::RectangleShape barra(sf::Vector2f(20, cpu.procesos[i].rafaga * 10));
            barra.setPosition(x, y - barra.getSize().y);
            barra.setFillColor(color);
            ventana.draw(barra);

            // Dibujar ID/nombre
            sf::Text textoID;
            textoID.setFont(fuente);
            // textoID.setString(cpu.procesos[i].nombre);
            textoID.setString(std::to_string(cpu.procesos[i].id));
            textoID.setCharacterSize(18);
            textoID.setFillColor(sf::Color::White);
            textoID.setPosition(x, y + 10);
            ventana.draw(textoID);
        }

        x += 50; // Separación entre barras
    }
}

void balancearCPUs(CPU& cpu1, CPU& cpu2, CPU& cpu3, CPU& cpu4) {
    std::vector<CPU*> cpus = {&cpu1, &cpu2, &cpu3, &cpu4};

    std::sort(cpus.begin(), cpus.end(), [](CPU* a, CPU* b) {
        return a->procesos.size() < b->procesos.size();
    });

    bool cambios = true;

    while (cambios) {
        cambios = false;

        CPU* cpuMenor = cpus.front();
        CPU* cpuMayor = cpus.back();

        if (cpuMayor->procesos.size() > cpuMenor->procesos.size() + 1) {
            Proceso procesoMovido = cpuMayor->procesos.back();
            cpuMenor->procesos.push_back(procesoMovido);
            cpuMayor->procesos.pop_back();

            // Actualizar IDs de los procesos en ambas CPUs
            /*
            for (size_t i = 0; i < cpuMenor->procesos.size(); ++i) {
                cpuMenor->procesos[i].id = i;
            }
            for (size_t i = 0; i < cpuMayor->procesos.size(); ++i) {
                cpuMayor->procesos[i].id = i;
            }
            */

            cambios = true;

            // Reordenar las CPUs por tamaño después de realizar un cambio
            std::sort(cpus.begin(), cpus.end(), [](CPU* a, CPU* b) {
                return a->procesos.size() < b->procesos.size();
            });
        }
    }
}

void DeleteporRango(std::vector<Proceso>& cpu1, std::vector<Proceso>& cpu2, std::vector<Proceso>& cpu3, std::vector<Proceso>& cpu4, int a, int b) {
    auto borrarEnRango = [a, b](std::vector<Proceso>& procesos) {
        procesos.erase(std::remove_if(procesos.begin(), procesos.end(), [a, b](const Proceso& p) {
            return p.id >= a && p.id <= b;
        }), procesos.end());
    };

    borrarEnRango(cpu1);
    borrarEnRango(cpu2);
    borrarEnRango(cpu3);
    borrarEnRango(cpu4);
}

int ubicarProceso(const std::vector<Proceso>& cpu1, const std::vector<Proceso>& cpu2, const std::vector<Proceso>& cpu3, const std::vector<Proceso>& cpu4, int id) {
    auto buscarEnCPU = [&id](const std::vector<Proceso>& procesos) {
        return std::any_of(procesos.begin(), procesos.end(), [&id](const Proceso& p) {
            return p.id == id;
        });
    };

    if (buscarEnCPU(cpu1)) {
        std::cout << "El proceso con ID " << id << " está en CPU 1.\n";
        return 1;
    }
    if (buscarEnCPU(cpu2)) {
        std::cout << "El proceso con ID " << id << " está en CPU 2.\n";
        return 2;
    }
    if (buscarEnCPU(cpu3)) {
        std::cout << "El proceso con ID " << id << " está en CPU 3.\n";
        return 3;
    }
    if (buscarEnCPU(cpu4)) {
        std::cout << "El proceso con ID " << id << " está en CPU 4.\n";
        return 4;
    }

    std::cout << "El proceso con ID " << id << " no se encuentra en ninguna CPU.\n";
    return -1;
}

int main() {
    // Crear las CPU con sus procesos
    
    CPU cpu1, cpu2, cpu3, cpu4;
    /*

    cpu1.agregarProceso("P1", 2, 0);
    cpu1.agregarProceso("P2", 4, 1);
    cpu1.agregarProceso("P3", 6, 2);
    cpu1.agregarProceso("P4", 8, 3);
    cpu1.agregarProceso("P5", 10, 4);
    cpu1.agregarProceso("PP1", 2, 0);
    cpu1.agregarProceso("PP2", 4, 1);
    cpu1.agregarProceso("PP3", 6, 2);
    cpu1.agregarProceso("PP4", 8, 3);
    cpu1.agregarProceso("PP5", 10, 4);

    cpu2.agregarProceso("P1", 1, 0);
    cpu2.agregarProceso("P2", 3, 1);
    cpu2.agregarProceso("P3", 5, 2);
    cpu2.agregarProceso("P4", 7, 3);
    cpu2.agregarProceso("P5", 8, 4);

    cpu3.agregarProceso("P1", 10, 0);
    cpu3.agregarProceso("P2", 9, 1);
    cpu3.agregarProceso("P3", 8, 2);
    cpu3.agregarProceso("P4", 7, 3);
    cpu3.agregarProceso("P5", 5, 4);

    cpu4.agregarProceso("P1", 10, 0);
    cpu4.agregarProceso("P2", 7, 1);
    cpu4.agregarProceso("P3", 4, 2);
    cpu4.agregarProceso("P4", 8, 3);
    cpu4.agregarProceso("P5", 9, 4);
    for (int i = 5; i < 400; i++)
    {
        cpu1.agregarProceso("P"+std::to_string(i), i%9+1, i);
    }
    */
    capturarProcesos(cpu1);


    // Crear ventana
    sf::RenderWindow ventana(sf::VideoMode(1200, 900), "Procesos de CPUs");

    sf::Clock reloj;
    sf::Clock colorReloj;
    
    
    //quantum del RR
    int quantum = 2;
    int localquantum = quantum;
    int pos = 0;

    // Pal de agregar
    sf::RectangleShape boton(sf::Vector2f(90, 50));
    boton.setFillColor(sf::Color::Green);
    boton.setPosition(900, 40);

    sf::Font fuente;
    fuente.loadFromFile("fonts/Arial.ttf");

    sf::Text textoBoton;
    textoBoton.setFont(fuente);
    textoBoton.setString("Agregar");
    textoBoton.setCharacterSize(18);
    textoBoton.setFillColor(sf::Color::White);
    textoBoton.setPosition(910, 50);

    //Pal de eliminar

    sf::RectangleShape botonEliminar(sf::Vector2f(90, 50));
    botonEliminar.setFillColor(sf::Color::Red);
    botonEliminar.setPosition(1000, 40);

    sf::Text textobotonEliminar;
    textobotonEliminar.setFont(fuente);
    textobotonEliminar.setString("Eliminar");
    textobotonEliminar.setCharacterSize(18);
    textobotonEliminar.setFillColor(sf::Color::White);
    textobotonEliminar.setPosition(1010, 50);

    //Pal de bUSCAR

    sf::RectangleShape botonBuscar(sf::Vector2f(90, 50));
    botonBuscar.setFillColor(sf::Color::Yellow);
    botonBuscar.setPosition(1100, 40);

    sf::Text textobotonBuscar;
    textobotonBuscar.setFont(fuente);
    textobotonBuscar.setString("Buscar");
    textobotonBuscar.setCharacterSize(18);
    textobotonBuscar.setFillColor(sf::Color::White);
    textobotonBuscar.setPosition(1110, 50);

    int procesoActual1 = -1, procesoActual2 = -1, procesoActual3 = -1, procesoActual4 = -1;
    int count = 0;
    while (ventana.isOpen()) {
        sf::Event evento;
        while (ventana.pollEvent(evento)) {
            if (evento.type == sf::Event::Closed)
                ventana.close();
            if (evento.type == sf::Event::MouseButtonPressed) {
                if (evento.type == sf::Event::MouseButtonPressed && evento.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f click(evento.mouseButton.x, evento.mouseButton.y);
                    if (boton.getGlobalBounds().contains(click)) {
                        std::string nombre;
                        
                        int rafaga;
                        int id;

                        std::cout << "Ingrese el nombre del proceso: ";
                        std::cin >> nombre;
                        std::cout << "Ingrese EL ID del proceso: ";
                        std::cin >> id;
                        std::cout << "Ingrese la ráfaga del proceso: ";
                        std::cin >> rafaga;

                        CPU* cpuMenor = &cpu1;
                        if (cpu2.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu2;
                        if (cpu3.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu3;
                        if (cpu4.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu4;

                        cpuMenor->agregarProceso(nombre, id, rafaga, reloj.getElapsedTime().asSeconds());
                    }
                    if (botonEliminar.getGlobalBounds().contains(click)) {
                        int a,b;
                        std::cout<<"Ingrese el rango a eliminar"<<std::endl;
                        std::cout<<"Primer valor (menor): ";
                        std::cin>>a;
                        std::cout<<std::endl;
                        std::cout<<"Segundo valor (mayor): ";
                        std::cin>>b;

                        DeleteporRango(cpu1.procesos, cpu2.procesos, cpu3.procesos, cpu4.procesos, a, b);
                        std::cout << "Procesos eliminados en el rango.\n";
                    }
                    if (botonBuscar.getGlobalBounds().contains(click)) {
                        int a;
                        std::cout<<"Ingrese el ID a buscar: ";
                        std::cin>>a;
                        ubicarProceso(cpu1.procesos, cpu2.procesos, cpu3.procesos, cpu4.procesos, a);
                    }
                        
                }
            }
        }

        // Actualizar cada segundo


        if (reloj.getElapsedTime().asSeconds() >= 1.0f) {
            if(localquantum==0){
                pos++;
                localquantum = quantum;
            }
            if(pos>=cpu4.procesos.size()){
                pos=0;
            }
            procesoActual1 = fifo(cpu1);
            procesoActual2 = sjf(cpu2);
            procesoActual3 = sjf(cpu3);
            procesoActual4 = rr(cpu4, pos);
            count++;
            localquantum--;

            balancearCPUs(cpu1,cpu2,cpu3,cpu4);
            reloj.restart();
            colorReloj.restart(); // Reiniciar reloj para control de color amarillo
        }

        ventana.clear();
        ventana.draw(boton);
        ventana.draw(textoBoton);

        ventana.draw(botonEliminar);
        ventana.draw(textobotonEliminar);

        ventana.draw(botonBuscar);
        ventana.draw(textobotonBuscar);

        sf::Text textoCPU1, textoCPU2, textoCPU3, textoCPU4;

        textoCPU1.setFont(fuente);
        textoCPU1.setString("CPU 1 - Proceso actual: " + (procesoActual1 >= 0 && procesoActual1 < static_cast<int>(cpu1.procesos.size()) ? cpu1.procesos[procesoActual1].nombre : "Ninguno"));
        textoCPU1.setCharacterSize(24);
        textoCPU1.setFillColor(sf::Color::White);
        textoCPU1.setPosition(10, 10);
        ventana.draw(textoCPU1);

        textoCPU2.setFont(fuente);
        textoCPU2.setString("CPU 2 - Proceso actual: " + (procesoActual2 >= 0 && procesoActual2 < static_cast<int>(cpu2.procesos.size()) ? cpu2.procesos[procesoActual2].nombre : "Ninguno"));
        textoCPU2.setCharacterSize(24);
        textoCPU2.setFillColor(sf::Color::White);
        textoCPU2.setPosition(10, 230);
        ventana.draw(textoCPU2);

        textoCPU3.setFont(fuente);
        textoCPU3.setString("CPU 3 - Proceso actual: " + (procesoActual3 >= 0 && procesoActual3 < static_cast<int>(cpu3.procesos.size()) ? cpu3.procesos[procesoActual3].nombre : "Ninguno"));
        textoCPU3.setCharacterSize(24);
        textoCPU3.setFillColor(sf::Color::White);
        textoCPU3.setPosition(10, 450);
        ventana.draw(textoCPU3);

        textoCPU4.setFont(fuente);
        textoCPU4.setString("CPU 4 - Proceso actual: " + (procesoActual4 >= 0 && procesoActual4 < static_cast<int>(cpu4.procesos.size()) ? cpu4.procesos[procesoActual4].nombre : "Ninguno"));
        textoCPU4.setCharacterSize(24);
        textoCPU4.setFillColor(sf::Color::White);
        textoCPU4.setPosition(10, 670);
        ventana.draw(textoCPU4);

        // Dibujar cada CPU
        dibujarCPU(ventana, cpu1, 0, "CPU1", colorReloj.getElapsedTime().asSeconds() < 0.25f ? procesoActual1 : -1);
        dibujarCPU(ventana, cpu2, 220, "CPU2", colorReloj.getElapsedTime().asSeconds() < 0.25f ? procesoActual2 : -1);
        dibujarCPU(ventana, cpu3, 440, "CPU3", colorReloj.getElapsedTime().asSeconds() < 0.25f ? procesoActual3 : -1);
        dibujarCPU(ventana, cpu4, 660, "CPU4", colorReloj.getElapsedTime().asSeconds() < 0.25f ? procesoActual4 : -1);

        ventana.display();

        // ESTABLECER TIEMPOS DE LLEGADA EN BASE AL RELOJ
    }

    return 0;
}