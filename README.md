# Alertas de tu pileta

Este proyecto es una página web estática diseñada para ayudar a una persona a registrar una clave de API de ThingSpeak para recibir alertas por mail cuando el nivel del agua de una pileta esté demasiado alto o demasiado bajo.

La idea general es simple: la persona ingresa una clave de tipo `TAK...`, la página la valida, la normaliza y la envía a ThingSpeak para que ese servicio la asocie con una variable que dispara una alerta por correo.

El proyecto combina HTML, JavaScript y lógica compilada en WebAssembly desde C++.

Link de la página publicada:
https://bessonebautista.github.io/pileta/cpp.html

## Objetivo de la página

La página no es un sistema completo de monitoreo con base de datos ni autenticación. Su propósito principal es ser una interfaz amigable para:

1. Explicar al usuario qué debe hacer en ThingSpeak.
2. Pedirle que pegue su clave de API de alertas.
3. Validar esa clave antes de enviarla.
4. Enviar la clave a la API de ThingSpeak para dejarla guardada en el canal asociado.
5. Mostrarle un mensaje visual de éxito o error.

## Cómo se ve la interfaz

La página presenta una estructura visual clara:

- Un encabezado con título y descripción.
- Un bloque con instrucciones paso a paso sobre qué hacer en ThingSpeak.
- Un formulario con un campo de texto donde el usuario pega la clave.
- Un botón para guardar.
- Un mensaje de estado que cambia según el resultado.

El diseño está hecho para ser amigable en celular, porque la idea es que la persona lo use desde el teléfono en el momento en que está gestionando su cuenta de ThingSpeak.

## Estructura del repositorio

El repositorio contiene estos archivos principales:

- `cpp.html`: la página visible, con HTML, CSS y JavaScript del front-end.
- `logica.cpp`: la lógica de validación y generación de la URL, escrita en C++.
- `logica.js`: el runtime generado por Emscripten para poder ejecutar el código C++ en el navegador.
- `logica.wasm`: el binario compilado a WebAssembly que ejecuta la lógica.

## Flujo completo de funcionamiento

### 1. La página se carga

Cuando el navegador abre `cpp.html`, se ejecuta el siguiente bloque:

```html
<script>
const WRITE_KEY = "1XSRVFYJZDUTK1RQ";

let recortarClave, validarClave, armarUrl;
var Module = {
  onRuntimeInitialized: function () {
    recortarClave = Module.cwrap("recortarClave", "string", ["string"]);
    validarClave  = Module.cwrap("validarClave", "number", ["string"]);
    armarUrl      = Module.cwrap("armarUrl", "string", ["string", "string"]);
  }
};
</script>
<script src="logica.js"></script>
```

Esto hace dos cosas importantes:

- Define la key de escritura de ThingSpeak (`WRITE_KEY`), que es la clave que identifica el canal o la configuración del usuario en ThingSpeak.
- Carga `logica.js`, que es el puente generado por Emscripten para cargar `logica.wasm` y exponer funciones C++ como si fueran funciones JavaScript.

El objeto `Module` se utiliza para crear wrappers con `cwrap()`, de forma que JavaScript pueda llamar a funciones exportadas desde C++.

### 2. Se prepara la conexión con el código compilado

Cuando el WebAssembly termina de inicializarse, el navegador deja disponible:

- `recortarClave(texto)`
- `validarClave(clave)`
- `armarUrl(writeKey, clave)`

Estas funciones vienen del archivo `logica.cpp` compilado en WebAssembly.

### 3. El usuario ingresa la clave

El formulario tiene un input del tipo texto:

```html
<input id="clave" type="text" ... placeholder="TAK..." required>
```

La idea es que el usuario copie una clave de API que ThingSpeak genera para alertas. Esa clave tiene que tener un formato similar a:

```text
TAKxxxxxxxxxxxxx
```

### 4. Al enviar el formulario

Cuando se presiona el botón, se ejecuta el `submit` del formulario:

```javascript
document.getElementById("formulario").addEventListener("submit", async function (e) {
  e.preventDefault();
  if (!validarClave) return mostrar("info", "Cargando... probá de nuevo en un segundo.");

  const clave = recortarClave(input.value);
  input.value = clave;
  const codigo = validarClave(clave);
  if (codigo === 1) return mostrar("error", "La clave tiene que empezar con <b>TAK</b>.");
  if (codigo !== 0) return mostrar("error", "La clave no parece completa o tiene espacios.");

  mostrar("info", "Guardando...");
  try {
    const respuesta = await fetch(armarUrl(WRITE_KEY, clave));
    const numero = (await respuesta.text()).trim();
    if (numero !== "0") {
      mostrar("ok", "✅ ¡Guardado! Ya vas a recibir los avisos de tu pileta en tu mail.");
    } else {
      mostrar("error", "No se pudo guardar. Esperá 15 segundos y probá de nuevo.");
    }
  } catch (error) {
    mostrar("error", "No se pudo conectar. Revisá tu internet.");
  }
});
```

Este bloque hace lo siguiente:

1. Evita la recarga normal del formulario.
2. Verifica que la lógica de C++ ya esté lista.
3. Llama a `recortarClave()` para sacar espacios al principio y al final.
4. Valida con `validarClave()`.
5. Si la clave es válida, arma la URL usando `armarUrl()`.
6. Envía un `fetch` a la API de ThingSpeak.
7. Lee la respuesta.
8. Muestra un mensaje al usuario según el resultado.

## Qué valida la lógica en C++

El archivo `logica.cpp` tiene la parte importante de la lógica. Veamos cada función.

### `recortarClave`

```cpp
const char* recortarClave(const char* texto) {
    std::string s = texto;
    while (!s.empty() && s.front() == ' ') s.erase(0, 1);
    while (!s.empty() && s.back() == ' ') s.pop_back();
    resultado = s;
    return resultado.c_str();
}
```

Esta función:

- Recibe el texto ingresado por el usuario.
- Elimina espacios al inicio y al final.
- Guarda una versión limpia en una variable global llamada `resultado`.
- La devuelve como un puntero a `char` para que JavaScript pueda usarla.

Es útil porque muchas veces el usuario copia la clave con espacios accidentales o al principio/final.

### `validarClave`

```cpp
int validarClave(const char* clave) {
    std::string c = clave;
    if (c.rfind("TAK", 0) != 0) return 1;
    if (c.find(' ') != std::string::npos) return 2;
    if (c.size() < 10) return 3;
    return 0;
}
```

La clave debe cumplir ciertas condiciones:

- Debe empezar con `TAK`.
- No debe tener espacios dentro.
- Debe tener una longitud mínima razonable.

Los valores de retorno son:

- `0`: válida.
- `1`: no empieza con `TAK`.
- `2`: tiene espacios.
- `3`: es demasiado corta.

Esto evita enviar una clave basura o incompleta a ThingSpeak.

### `armarUrl`

```cpp
const char* armarUrl(const char* writeKey, const char* clave) {
    std::string claveOk;
    for (char c : std::string(clave)) {
        if (c == '+') claveOk += "%2B";
        else claveOk += c;
    }
    resultado = std::string("https://api.thingspeak.com/update?")
              + "api_key=" + writeKey + "&field1=" + claveOk;
    return resultado.c_str();
}
```

La función arma una URL del tipo:

```text
https://api.thingspeak.com/update?api_key=WRITE_KEY&field1=TAKxxxxx
```

Esto es importante porque la API de ThingSpeak espera que la clave se envíe como parámetro en la URL.

Además, reemplaza `+` por `%2B`, porque en una URL un signo más puede ser interpretado como un carácter especial si no se codifica bien.

## Relación con ThingSpeak

ThingSpeak es un servicio de Internet de Things que permite conectar sensores y dispositivos con un canal. En este caso, la clave que se guarda se usa para actualizar un campo de ese canal.

La página no tiene un backend propio. En vez de eso, manda la request directamente desde el navegador a:

```text
https://api.thingspeak.com/update
```

con los parámetros:

- `api_key`: la clave de escritura del canal
- `field1`: la clave de alertas del usuario

Si la respuesta del servidor no es `0`, significa que la actualización tuvo éxito.

La lógica de la página está pensada para que el usuario reciba un mail cuando el nivel del agua se salga de cierto rango, siempre y cuando el sistema externo de ThingSpeak esté configurado para eso.

## ¿Qué significa realmente “guardar la clave”?

La clave no se almacena en un servidor de esta página; la clave se envía a la API de ThingSpeak para que el canal la registre como un valor asociado a una variable.

En otras palabras:

- La página valida el formato.
- La página prepara la petición.
- ThingSpeak procesa la información.
- La aplicación de alertas puede leer ese valor y disparar correos.

Este diseño simplifica la arquitectura y reduce la necesidad de manejar una base de datos propia.

## ¿Por qué usar C++ + WebAssembly?

La decisión de escribir la lógica crítica en C++ y compilarla a WebAssembly tiene varias ventajas:

- La validación es más segura y más estable que un script JavaScript demasiado complejo.
- El código puede quedar encapsulado en funciones bien definidas.
- Se logra una pequeña “capa de lógica” que puede ejecutarse dentro del navegador sin un servidor de procesamiento.
- Demuestra cómo un algoritmo tradicional puede reutilizarse en una aplicación web moderna.

El archivo `logica.js` es un artefacto generado por Emscripten. No es código escrito a mano para la lógica específica del negocio; es el entorno necesario para ejecutar el módulo compilado.

## Mapa conceptual del sistema

```text
Usuario
  ↓
HTML + CSS + JS (cpp.html)
  ↓
Validación con C++/WebAssembly
  ↓
URL formada con api_key + field1
  ↓
ThingSpeak API
  ↓
Canal / variable configurada para alertas
  ↓
Mail de aviso
```

## Cómo probar la página localmente

Para abrir la página en un navegador, basta con localizar `cpp.html` y abrirlo directamente.

Si se quiere probar en un servidor local, puede usarse cualquier servidor estático simple, por ejemplo:

```bash
python -m http.server 8000
```

Luego abrir en el navegador:

```text
http://localhost:8000/cpp.html
```

## Consideraciones de seguridad y uso

- La clave API se guarda en un entorno externo, no en una base de datos de esta página.
- La validación ayuda a evitar errores de formato, pero no reemplaza un manejo completo de seguridad.
- La página depende de la disponibilidad de Internet para poder conectar con ThingSpeak.
- Si no hay conexión o si ThingSpeak no responde, la operación puede fallar con mensajes claros para el usuario.

## Limitaciones

Esta página tiene un alcance muy concreto:

- No almacena usuarios.
- No tiene login.
- No tiene base de datos local.
- No gestiona autenticación ni permisos complejos.
- Está pensada para una operación simple: ingresar una clave y enviarla a ThingSpeak.

## Resumen técnico

En resumen, la aplicación funciona así:

- La interfaz le indica al usuario qué debe hacer en ThingSpeak.
- El usuario pega una clave `TAK...`.
- JavaScript llama a funciones exportadas desde C++ vía WebAssembly.
- El código C++ recorta la clave, valida el formato y arma la URL.
- La URL se envía con `fetch()` a la API de ThingSpeak.
- Si la respuesta no es `0`, la operación se considera exitosa y se muestra un mensaje al usuario.

## Conclusión

Esta página es un buen ejemplo de un proyecto pequeño pero muy didáctico: combina front-end, lógica en C++, WebAssembly y una API externa para crear una experiencia útil y muy concreta para una persona que quiere recibir alertas por mail.

Es una solución bastante simple desde el punto de vista de infraestructura, pero muy clara desde el punto de vista de aprendizaje porque permite ver de manera práctica cómo se conectan:

- interfaz gráfica,
- validación de entrada,
- compilación de código nativo al navegador,
- servicios externos,
- y flujo completo de una acción cotidiana de una aplicación web.

## Nota para un profesor o tutor

Si un profesor quiere evaluar este trabajo, puede entenderse como una pequeña aplicación web de integración con un servicio externo. La parte más importante no es solo que “funciona”, sino que demuestra comprensión de conceptos clave como:

- interacción usuario-HTML,
- validación de datos,
- uso de APIs REST,
- integración entre JavaScript y WebAssembly,
- separación de responsabilidades entre la interfaz y la lógica del negocio.

Eso hace que el proyecto sea útil tanto como ejercicio técnico como ejemplo pedagógico de desarrollo web con lógica compilada.
