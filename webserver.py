from http.server import BaseHTTPRequestHandler, HTTPServer ### 导入必要的模块和依赖
import os
from os import path, system, environ
from urllib.parse import urlparse
import pymysql as pms
import cgi
import time

### MIME-TYPE
mimedic = {
    '.html': 'text/html',
    '.htm': 'text/html',
    '.js': 'application/javascript',
    '.css': 'text/css',
    '.json': 'application/json',
    '.png': 'image/png',
    '.jpg': 'image/jpg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.txt': 'text/plain',
    '.avi': 'video/x-msvideo',
}

g_db_address = "localhost"
g_db_user = "lor"
g_db_pwd = "123456"
g_db_name = "DemoDB"

curdir = path.dirname(path.realpath(__file__))
sep = '/'

class server_handler(BaseHTTPRequestHandler):
    def do_GET(self):
        # mimetype = 'text/html'
        # page_file = open("./index.html", 'rb')
        # self.send_response(200)
        # self.send_header('Content-type', mimetype)
        # self.end_headers()
        # self.wfile.write(page_file.read())
        url_parts = urlparse(self.path)
        file_path = url_parts.path

        if file_path.endswith('/'):
            file_path += 'index.html'

        _, suffix = os.path.splitext(file_path)
        if suffix in mimedic.keys():
            with open(os.path.realpath(curdir + sep + file_path), 'rb') as f:
                content = f.read()
                self.send_response(200)
                self.send_header('Content-type', mimedic[suffix])
                self.end_headers()
                self.wfile.write(content)

    def do_POST(self):
        self.parent_dir = path.dirname(path.abspath(__file__))
        mimetype="text/html"
        form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={
                    'REQUEST_METHOD' : 'POST',
                    'CONTENT_TYPE' : self.headers['Content-Type'],
                }
        )
        print(form['login_user'].value)
        print(form['login_passwd'].value)

        db_conn = pms.connect(host=g_db_address, user=g_db_user, password=g_db_pwd, database=g_db_name)
        db_cursor = db_conn.cursor()
        select_cmd = "SELECT passwd from Users where name=\"" + form['login_user'].value + "\""
        db_cursor.execute(select_cmd)
        row = db_cursor.fetchone()
        print(row)
        if form['login_passwd'].value == row[0]:
            monitor_file = open(os.path.join(self.parent_dir, self.path[1:]) + "monitor.html", "rb")
        else:
            pass

        self.send_response(200)
        self.send_header('Content-type', mimetype)
        self.end_headers()
        self.wfile.write(monitor_file.read())

def run():
    port = 8080
    print("Starting server, port ", port)

    ### Server settings
    server_address = ('', port)
    httpd = HTTPServer(server_address, server_handler)
    print("Server is running ...")
    httpd.serve_forever()

if __name__ == '__main__':
    run()
