#!/usr/bin/python
import cgi, cgitb

cgitb.enable()

form = cgi.FieldStorage(encoding='utf-8')
name = form.getvalue('name')

print('Content-type: text/html;charset=utf-8')
print()

print(f'''
    <head>
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Hello {name}</title>
    </head>
      ''')

print(f'<h1>Hello, {name}</h1>')