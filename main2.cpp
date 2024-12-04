#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
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

    void agregarProceso(const std::string& nombre, int rafaga, int tiempoLlegada) {
        procesos.emplace_back(nombre, procesos.size(), rafaga, tiempoLlegada);
    }
};

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
            for (size_t i = 0; i < cpu.procesos.size(); ++i) {
                cpu.procesos[i].id = i; 
            }
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
        if (cpu.procesos[0].rafaga <= 0) {
            cpu.procesos.erase(cpu.procesos.begin()); 
            for (size_t i = 0; i < cpu.procesos.size(); ++i) {
                cpu.procesos[i].id = i; 
            }
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
            textoID.setString(cpu.procesos[i].nombre);
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
            for (size_t i = 0; i < cpuMenor->procesos.size(); ++i) {
                cpuMenor->procesos[i].id = i;
            }
            for (size_t i = 0; i < cpuMayor->procesos.size(); ++i) {
                cpuMayor->procesos[i].id = i;
            }

            cambios = true;

            // Reordenar las CPUs por tamaño después de realizar un cambio
            std::sort(cpus.begin(), cpus.end(), [](CPU* a, CPU* b) {
                return a->procesos.size() < b->procesos.size();
            });
        }
    }
}


int main() {
    // Crear las CPU con sus procesos
    CPU cpu1, cpu2, cpu3, cpu4;

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
    

    // Crear ventana
    sf::RenderWindow ventana(sf::VideoMode(1200, 900), "Procesos de CPUs");

    sf::Clock reloj;
    sf::Clock colorReloj;
    
    
    //quantum del RR
    int quantum = 2;
    int localquantum = quantum;
    int pos = 0;

    sf::RectangleShape boton(sf::Vector2f(200, 50));
    boton.setFillColor(sf::Color::Green);
    boton.setPosition(900, 50);

    sf::Font fuente;
    fuente.loadFromFile("fonts/Arial.ttf");

    sf::Text textoBoton;
    textoBoton.setFont(fuente);
    textoBoton.setString("Agregar Proceso");
    textoBoton.setCharacterSize(18);
    textoBoton.setFillColor(sf::Color::White);
    textoBoton.setPosition(910, 60);

    int procesoActual1 = -1, procesoActual2 = -1, procesoActual3 = -1, procesoActual4 = -1;
    int count = 0;
    while (ventana.isOpen()) {
        sf::Event evento;
        while (ventana.pollEvent(evento)) {
            if (evento.type == sf::Event::Closed)
                ventana.close();
            if (evento.type == sf::Event::MouseButtonPressed) {
                if (boton.getGlobalBounds().contains(evento.mouseButton.x, evento.mouseButton.y)) {
                    std::string nombre;
                    int rafaga;

                    std::cout << "Ingrese el nombre del proceso: ";
                    std::cin >> nombre;
                    std::cout << "Ingrese la ráfaga del proceso: ";
                    std::cin >> rafaga;

                    CPU* cpuMenor = &cpu1;
                    if (cpu2.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu2;
                    if (cpu3.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu3;
                    if (cpu4.procesos.size() < cpuMenor->procesos.size()) cpuMenor = &cpu4;

                    cpuMenor->agregarProceso(nombre, rafaga, reloj.getElapsedTime().asSeconds());
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
