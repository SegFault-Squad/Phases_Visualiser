from flask import Flask, render_template, request
import subprocess
import tempfile
import os

app = Flask(__name__, static_folder='static')

@app.route('/', methods=['GET', 'POST'])
def index():
    output = ""
    selected_phase = ""
    code = ""

    if request.method == 'POST':
        code = request.form['code']
        selected_phase = request.form['phase']

        with tempfile.NamedTemporaryFile(delete=False, suffix=".cpp", mode='w') as tmp:
            tmp.write(code)
            tmp_path = tmp.name

        binary_path = './analyzer'
        if not os.path.exists(binary_path):
            compile_process = subprocess.run(
                ['g++', 'analyzer.cpp', '-o', binary_path],
                cwd='backend',
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            if compile_process.returncode != 0:
                output = compile_process.stderr
                return render_template('index.html', code=code, output=output, phase=selected_phase)

        run_process = subprocess.run(
            [binary_path, selected_phase, tmp_path],
            cwd='backend',
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        output = run_process.stdout if run_process.returncode == 0 else run_process.stderr
        os.remove(tmp_path)

    return render_template('index.html', code=code, output=output, phase=selected_phase)

if __name__ == '__main__':
    app.run(debug=True)
