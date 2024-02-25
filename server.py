#deploy this to some lcoud hosted server (for example on heroku), the esp32 will not see it in the localhost
#put the following template in the templates folder besides this file
from flask import Flask, request, render_template
import logging
import os
import base64  
from PIL import Image
import numpy as np
import io 
from PIL import Image, ImageDraw
import cv2
import pickle
import face_recognition
import numpy
import cvzone

import sys
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication

from facenet_pytorch import MTCNN, InceptionResnetV1
import torch






counter = {'kamrul': 0, 'zawad': 0, 'rasel': 0} # counts number of detection
def clean_counter(counter):
    for k in counter:
        counter[k] = 0

# bring encoded data:
load_data = torch.load('./encoded_data.pt')
embedding_list = load_data[0]
name_list = load_data[1]



######################### model #####################

mtcnn0 = MTCNN(
    image_size=160, margin=0, min_face_size=20,
    thresholds=[0.6, 0.7, 0.7], factor=0.709, post_process=True,
    device='cpu'
)
resnet = InceptionResnetV1(pretrained='vggface2').eval()


######################################################


################## server  #############################

global new_disturbance
new_disturbance = True
capture_count = 0 # number of image per disturbance
threshold = 0.77 # enbedded avg distance

app = Flask(__name__)

logging.basicConfig(level=logging.DEBUG) #for better debuging, we will log out every request with headers and body.
@app.before_request
def log_request_info():
    logging.info('Headers: %s', request.headers)
    logging.info('Body: %s', request.get_data())

UPLOAD_FOLDER = './static/uploads'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

@app.route("/image", methods=["GET", "POST"])
def upload_image():

    if request.method == "POST": #if we make a post request to the endpoint, look for the image in the request body
        image_raw_bytes = request.data #get_data()  #get the whole body


        # save_location = (os.path.join(app.root_path, "static/test.jpg")) #save to the same folder as the flask app live in 

        # f = open(save_location, 'wb') # wb for write byte data in the file instead of string
        # f.write(image_raw_bytes) #write the bytes from the request body to the file
        # f.close()
        # convert it into bytes 
        image = Image.open(io.BytesIO(image_raw_bytes))
        image.show()
        image_path = os.path.join(app.config['UPLOAD_FOLDER'], 'to_send.jpeg')
        image.save(image_path)

        # if new_disturbance:
        #     clean_counter(counter)  # refresh for each image
        #     new_disturbance = False
        # capture_count += 1

        face, prob = mtcnn0(image, return_prob=True)
        if face is not None and prob > 0.92:  # face found
            emb = resnet(face.unsqueeze(0))
            dist_list = []
            for emb_ in embedding_list:
                dist = torch.dist(emb, emb_).item()
                dist_list.append(dist)
            vals, indices = torch.topk(torch.tensor(dist_list), 7,  largest=False)
            names = []
            for min_idx in indices:
                name = name_list[min_idx]
                counter[name] += 1
                names.append(name)
            max_face = max(counter, key=counter.get)
            if torch.mean(vals) < threshold:  # good confidence
                print("Chill its "+ max_face)
                ############# mailing code #####################
                # Email configuration
                sender_email = "XXXXXXXXXXXXXXXXXX"
                receiver_email = "XXXXXXXXXXXXXXXXXXX"
                subject = "Warehourse security alert"
                body = max_face + "was here."

                # Create the email message
                message = MIMEMultipart()
                message['From'] = sender_email
                message['To'] = receiver_email
                message['Subject'] = subject
                message.attach(MIMEText(body, 'plain'))

                # Attach the image file
                with open(image_path, "rb") as attachment:
                    part = MIMEApplication(attachment.read(), Name=image_path)
                    part['Content-Disposition'] = f'attachment; filename="{image_path}"'
                    message.attach(part)

                # Connect to the SMTP server and send the email
                with smtplib.SMTP("smtp.gmail.com", 587) as server:
                    server.starttls()
                    server.login(sender_email, "XXXXXXXXXXXXXXX")
                    server.sendmail(sender_email, receiver_email, message.as_string())

                
                # capture_count = 0
                # new_disturbance = True 
                return b'S'   #stop and sleep
            else:
                print('Its an intruder')
                # Email configuration
                sender_email = "XXXXXXXXXXXXXXXXXXXX"
                receiver_email = "XXXXXXXXXXXXXXXXXXXXXX"
                subject = "Warehourse security alert"
                body = "It is probably an intruder"

                # Create the email message
                message = MIMEMultipart()
                message['From'] = sender_email
                message['To'] = receiver_email
                message['Subject'] = subject
                message.attach(MIMEText(body, 'plain'))

                # Attach the image file
                with open(image_path, "rb") as attachment:
                    part = MIMEApplication(attachment.read(), Name=image_path)
                    part['Content-Disposition'] = f'attachment; filename="{image_path}"'
                    message.attach(part)

                # Connect to the SMTP server and send the email
                with smtplib.SMTP("smtp.gmail.com", 587) as server:
                    server.starttls()
                    server.login(sender_email, "XXXXXXXXXXXXXXXXXXX")
                    server.sendmail(sender_email, receiver_email, message.as_string())

                return b'S'
        else:  # if not found: send another image
            return b'X'
            if capture_count == 3:
                capture_count = 0
                new_disturbance = True
                return b'S'   # stop and sleep 
            else:    #send another capture
                return b'A'
            



        


    if request.method == "GET": #if we make a get request (for example with a browser) show the image.
# The browser will cache this image so when you want to actually refresh it, press ctrl-f5
        return b'hi'
    return "if you see this, that is bad request method"


def run_server_api():
    app.run(host='0.0.0.0', port=8080)
  
  
if __name__ == "__main__":     
    run_server_api()