This folder contains certificates of the HTTPs servers storing OTA update components.

Assumming the OTA components are to be downloaded from public bucket(s) of Amazon S3 server, certificate of the HTTPs server can be obtained as below:

    openssl s_client -showcerts -connect s3.amazonaws.com:443 < /dev/null
