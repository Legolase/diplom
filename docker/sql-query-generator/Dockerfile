FROM python:3.7.2-alpine3.8
COPY . /app
RUN pip install -r /app/requirements.txt
ENTRYPOINT ["python", "-u", "./app/generator.py"]