<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Plataforma Mensajes</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 20px;
        }
        h1, h2, h3 {
            color: #2c3e50;
        }
        ul {
            list-style-type: none;
            padding: 0;
        }
        li {
            margin-bottom: 10px;
        }
        code {
            background-color: #ecf0f1;
            padding: 2px 4px;
            border-radius: 4px;
        }
    </style>
</head>
<body>

    <h1>Plataforma Mensajes</h1>

    <h2>Funcionalidades</h2>

    <h3>Servidor (administrado por el manager)</h3>
    <ul>
        <li>
            <strong>1. Listar usuarios que están usando la plataforma actualmente</strong><br>
            Comando: <code>users</code><br>
            Muestra la lista de usuarios activos en la plataforma.
        </li>
        <li>
            <strong>2. Eliminar un usuario</strong><br>
            Comando: <code>remove &lt;nombre_de_usuario&gt;</code><br>
            Permite eliminar un usuario de la plataforma.
        </li>
        <li>
            <strong>3. Listar los temas en la plataforma</strong><br>
            Comando: <code>topics</code><br>
            Muestra el nombre de los temas existentes y la cantidad de mensajes persistentes en cada uno.
        </li>
        <li>
            <strong>4. Listar los mensajes de un tema específico</strong><br>
            Comando: <code>show &lt;tema&gt;</code><br>
            Muestra todos los mensajes persistentes de un tema dado.
        </li>
        <li>
            <strong>5. Bloquear un tema</strong><br>
            Comando: <code>lock &lt;tema&gt;</code><br>
            Bloquea el envío de nuevos mensajes al tema indicado.
        </li>
        <li>
            <strong>6. Desbloquear un tema</strong><br>
            Comando: <code>unlock &lt;tema&gt;</code><br>
            Permite nuevamente el envío de mensajes al tema bloqueado.
        </li>
        <li>
            <strong>7. Cerrar la plataforma</strong><br>
            Comando: <code>close</code><br>
            Permite cerrar la plataforma.
        </li>
    </ul>

    <h3>Cliente</h3>
    <ul>
        <li>
            <strong>1. Obtener una lista de todos los temas</strong><br>
            Comando: <code>topics</code><br>
            Muestra el nombre de los temas existentes, el número de mensajes persistentes en cada uno y su estado (bloqueado/desbloqueado).
        </li>
        <li>
            <strong>2. Enviar un mensaje a un tema específico</strong><br>
            Comando: <code>msg &lt;tema&gt; &lt;duración&gt; &lt;mensaje&gt;</code><br>
            Permite a un cliente el envío de mensajes en temas. No hace falta estar suscrito para ejecutar el comando.
        </li>
        <li>
            <strong>3. Suscribirse a un tema</strong><br>
            Comando: <code>subscribe &lt;tema&gt;</code><br>
            Permite a un cliente suscribirse a un determinado tema y poder recibir mensajes de ese tema.
        </li>
        <li>
            <strong>4. Darse de baja de un tema específico</strong><br>
            Comando: <code>unsubscribe &lt;tema&gt;</code><br>
            Permite a un cliente desuscribirse de un tema.
        </li>
        <li>
            <strong>5. Salir, terminando el proceso de feed</strong><br>
            Comando: <code>exit</code><br>
            Permite a un cliente salir de la plataforma.
        </li>
    </ul>

</body>
</html>
