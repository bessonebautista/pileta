// logica.cpp - Lógica de la página "Alertas de tu pileta" en C++
// Se compila a WebAssembly con Emscripten (ver comando abajo).
//
// Comando para compilar:
//   emcc logica.cpp -O2 -o logica.js --no-entry "-sEXPORTED_FUNCTIONS=_recortarClave,_validarClave,_armarUrl" "-sEXPORTED_RUNTIME_METHODS=cwrap"

#include <string>
#include <cctype>

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#else
  #define EMSCRIPTEN_KEEPALIVE   // para probar en la compu con g++
#endif

// Guardamos el resultado en una variable global para que el texto
// siga existiendo cuando JavaScript lo lea.
static std::string resultado;

// Saca los espacios del principio y del final.
static std::string recortar(const std::string& s) {
    size_t inicio = 0;
    size_t fin = s.size();
    while (inicio < fin && std::isspace((unsigned char)s[inicio])) inicio++;
    while (fin > inicio && std::isspace((unsigned char)s[fin - 1])) fin--;
    return s.substr(inicio, fin - inicio);
}

// Convierte un texto a formato de URL: el "+" pasa a %2B, etc.
static std::string codificar(const std::string& s) {
    const char* hex = "0123456789ABCDEF";
    std::string salida;
    for (unsigned char c : s) {
        bool seguro = std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';
        if (seguro) {
            salida += (char)c;
        } else {
            salida += '%';
            salida += hex[c >> 4];
            salida += hex[c & 15];
        }
    }
    return salida;
}

extern "C" {

// Devuelve la clave sin espacios al principio ni al final.
EMSCRIPTEN_KEEPALIVE
const char* recortarClave(const char* texto) {
    resultado = recortar(texto);
    return resultado.c_str();
}

// Devuelve 0 si la clave está bien, o un código de error:
//   1 = no empieza con TAK
//   2 = tiene espacios en el medio
//   3 = el largo no es razonable
EMSCRIPTEN_KEEPALIVE
int validarClave(const char* clave) {
    std::string c = clave;
    if (c.rfind("TAK", 0) != 0) return 1;
    for (char ch : c) {
        if (std::isspace((unsigned char)ch)) return 2;
    }
    if (c.size() < 10 || c.size() > 40) return 3;
    return 0;
}

// Arma la dirección que se manda a ThingSpeak.
EMSCRIPTEN_KEEPALIVE
const char* armarUrl(const char* writeKey, const char* clave) {
    resultado = "https://api.thingspeak.com/update?api_key=" + codificar(writeKey)
              + "&field1=" + codificar(clave);
    return resultado.c_str();
}

}  // extern "C"

// ---- Solo para probar en la compu (no se usa en la página) ----
#ifndef __EMSCRIPTEN__
#include <iostream>
int main() {
    std::string entrada;
    std::cout << "Pegá la clave: ";
    std::getline(std::cin, entrada);
    std::string clave = recortarClave(entrada.c_str());
    int codigo = validarClave(clave.c_str());
    if (codigo != 0) {
        std::cout << "Error, código " << codigo << "\n";
        return 1;
    }
    std::cout << armarUrl("1XSRVFYJZDUTK1RQ", clave.c_str()) << "\n";
    return 0;
}
#endif
