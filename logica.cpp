// logica.cpp - Lógica de la página "Alertas de tu pileta"
#include <string>
#include <emscripten.h>

// Acá se guarda el texto que se le devuelve a JavaScript
static std::string resultado;

extern "C" {

// 1) Saca los espacios del principio y del final
EMSCRIPTEN_KEEPALIVE
const char* recortarClave(const char* texto) {
    std::string s = texto;
    while (!s.empty() && s.front() == ' ') s.erase(0, 1);
    while (!s.empty() && s.back() == ' ') s.pop_back();
    resultado = s;
    return resultado.c_str();
}

// 2) Revisa la clave. Devuelve: 0 = bien, 1 = no empieza con TAK,
//    2 = tiene espacios en el medio, 3 = muy corta
EMSCRIPTEN_KEEPALIVE
int validarClave(const char* clave) {
    std::string c = clave;
    if (c.rfind("TAK", 0) != 0) return 1;
    if (c.find(' ') != std::string::npos) return 2;
    if (c.size() < 10) return 3;
    return 0;
}

// 3) Arma la dirección para ThingSpeak. El "+" se reemplaza por %2B
EMSCRIPTEN_KEEPALIVE
const char* armarUrl(const char* writeKey, const char* clave) {
    std::string claveOk;
    for (char c : std::string(clave)) {
        if (c == '+') claveOk += "%2B";
        else claveOk += c;
    }
    resultado = std::string("https://api.thingspeak.com/update?api_key=")
              + writeKey + "&field1=" + claveOk;
    return resultado.c_str();
}

}
