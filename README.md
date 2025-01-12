# PlataformaMensajes

## Ejecución

Para generar los archivos de la plataforma y limpiar los existentes, puedes usar los siguientes comandos:

1. **Limpiar los archivos generados anteriormente**  
   Comando:  
   ```bash
   make clean
2. **Generar los archivos nuevamente**  
   Comando:  
   ```bash
   make
   
## Funcionalidades

### Servidor (administrado por el manager)

1. **Listar usuarios que están usando la plataforma actualmente**  
   Comando: `users`  
   Muestra la lista de usuarios activos en la plataforma.

2. **Eliminar un usuario**  
   Comando: `remove <nombre_de_usuario>`  
   Permite eliminar un usuario de la plataforma.

3. **Listar los temas en la plataforma**  
   Comando: `topics`  
   Muestra el nombre de los temas existentes y la cantidad de mensajes persistentes en cada uno.

4. **Listar los mensajes de un tema específico**  
   Comando: `show <tema>`  
   Muestra todos los mensajes persistentes de un tema dado.

5. **Bloquear un tema**  
   Comando: `lock <tema>`  
   Bloquea el envío de nuevos mensajes al tema indicado.

6. **Desbloquear un tema**  
   Comando: `unlock <tema>`  
   Permite nuevamente el envío de mensajes al tema bloqueado.

7. **Cerrar la plataforma**  
   Comando: `close`  
   Permite cerrar la plataforma.

### Cliente

1. **Obtener una lista de todos los temas**  
   Comando: `topics`  
   Muestra el nombre de los temas existentes, el número de mensajes persistentes en cada uno y su estado (bloqueado/desbloqueado).

2. **Enviar un mensaje a un tema específico**  
   Comando: `msg <tema> <duración> <mensaje>`  
   Permite a un cliente el envío de mensajes en temas. No hace falta estar suscrito para ejecutar el comando.

3. **Suscribirse a un tema**  
   Comando: `subscribe <tema>`  
   Permite a un cliente suscribirse a un determinado tema y poder recibir mensajes de ese tema.

4. **Darse de baja de un tema específico**  
   Comando: `unsubscribe <tema>`  
   Permite a un cliente desuscribirse de un tema.

5. **Salir, terminando el proceso de feed**  
   Comando: `exit`  
   Permite a un cliente salir de la plataforma.
