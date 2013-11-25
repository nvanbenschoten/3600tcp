from subprocess import call
from time import sleep
import smtplib
import ConfigParser
import ast

def remove_failure(find, items):
    i = 0
    for fail in items:
        if fail['main'] == find['main'] and fail['sub'] == find['sub']:
            del(items[i])
            print("Found and removed outdated failure\n")
            #return True
        else:
            i += 1;
    #return -1
    

def get_leaderboard(results_file):
    # run leaderboard script and write current results to a text file
    ret = call(["ssh", "nat@cs3600tcp.ccs.neu.edu", '"/course/cs3600f13/bin/project4/printstats"', ">", results_file])
    if ret == 0:
        return True
    else:
        return False
    
def parse_leaderboard(results_file):
    # open results file and read lines to list
    with open(results_file) as f:
        leaderboard = f.readlines();
        f.close()

    i = 0;
    # parse list looking for other people in 1st
    while i < len(leaderboard):
        line = leaderboard[i]
        if "-----" in line:
            test_details = line.replace("----- ", "").replace(" -----", "").replace("\n", "")

        #print line[0:2]
        if line[0:2] == "1:": # if we are at the leader
            # get the type of the subtest
            subtest = leaderboard[i-1].replace(":", "")
            # get the leader's username
            leader = line.split()[0].split(":")[1] 
            if leader not in usernames: # if we are not the leader
                # find our rank 
                j = i+1
                while 1:
                    user = leaderboard[j].split()[0].split(":")[1]
                    rank = leaderboard[j].split()[0].split(":")[0]
                    if user in usernames:
                        break
                    j += 1
                # get the type of the subtest
                #subtest = leaderboard[i-1].replace(":", "")
                # send a text message alert that we are failing if not already sent
                fail = {'main':test_details, 'sub':subtest, 'rank':rank}
                if fail not in test_failures:
                    test_failures.append(fail)
                    print("Found a test failure:\n")
                    print(fail)
                    textAlert(test_details, subtest, rank)
            else:
                #subtest = leaderboard[i-1].replace(":", "")
                fail = {'main':test_details, 'sub':subtest, 'rank':1}
                remove_failure(fail, test_failures)



        i += 1;

def textAlert(test, subtest, rank):
    server = smtplib.SMTP("smtp.gmail.com", 587)
    server.ehlo()
    server.starttls()
    server.login(gmail_info['username'], gmail_info['password'])
    message = "You are currently ranked %d for %s - %s" % (int(rank), test, subtest)
    for a in phonenum_emails:
        body = '\r\n'.join(['To: %s' % a,
                            'From: %s' % gmail_info['username'],
                            'Subject: 3600tcp Alert',
                            '', message])
        server.sendmail(gmail_info['username'], a, body)
        print "Sent from %s to %s with message:\n" % (gmail_info['username'], a)
        print message
    server.quit()

# read in sensitive info from settings file
config_file = 'settings.ini'
configp = ConfigParser.RawConfigParser()
configp.read(config_file)
gmail_info = ast.literal_eval(configp.get('leaderboard', 'gmailinfo'))
phonenum_emails = configp.get('leaderboard', 'phonenumemails').split(',')
print gmail_info
print type(gmail_info)
print phonenum_emails
print type(phonenum_emails)
# setup other useful things
results = "current_results.txt"
usernames = ["nvanben", "nat"]
test_failures = []
wait_minutes = 3;

# main loop to check leaderboard
while 1:
    get_leaderboard(results)
    parse_leaderboard(results)
    sleep(60 * wait_minutes)
