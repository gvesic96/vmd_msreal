make
sudo ./main
sudo ln /dev/null /dev/raw1394
python draw_img.py
echo "-----------------------------------------------"
echo "Starting Simple HTTP Server on Local Network..."
echo "PORT=8080"
echo "-----------------------------------------------"
python -m SimpleHTTPServer 8080
