import java.io.*;
import java.util.*;

public class Verifier {
    public Boolean verifyLog(String txtdir, String nof, String property, int threshold) {
        boolean ret = false;
        
        switch(property) {
            case "operationTime":
                ret = operationTimeVerification(txtdir, nof, threshold);
                break;
                
            case "operationSuccessRate":
                ret = operationSuccessRateVerification(txtdir, nof, threshold);
//                File file = new File(System.getProperty("user.dir") + "/StarPlateS/SoS_Extension/Verification_Results.txt");
                File file = new File(System.getProperty("user.dir") + "/StarPlateS/SoS_Extension/Verification_Results" + nof + "_" + threshold + ".csv");
                FileWriter writer = null;
                try {
                    writer = new FileWriter(file, true);
//                    writer.write("operationSuccessRate for " + txtdir.replace(System.getProperty("user.dir") + "/StarPlateS/SoS_Extension/", "") + " is " + Boolean.toString(ret) + "\n");
                    writer.write(txtdir.replace(System.getProperty("user.dir") + "/StarPlateS/SoS_Extension/", "") + "," + Boolean.toString(ret) + "\n");
                    writer.flush();
                } catch(IOException e) {
                    e.printStackTrace();
                } finally {
                    try {
                        if(writer != null) writer.close();
                    } catch(IOException e) {
                        e.printStackTrace();
                    }
                }
//                System.out.println(ret);
                break;
        }
        
        return ret;
    }
    
    private Boolean operationTimeVerification(String txtdir, String nof, int threshold) {
        boolean ret = true;
    
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(new File(txtdir)));
        
            String line;
            String startEnd;
            String vehId;
            String rcvPltId;
            float time;
            ArrayList<Message> messages = new ArrayList<>();
        
            while ((line = reader.readLine()) != null) {
            
                StringTokenizer st = new StringTokenizer(line, "\t ");
            
                if (st.countTokens() == 9) {
                    String temp = st.nextToken();
                    if(temp.equals("timeStep")) continue;
                    
                    time = Float.valueOf(temp);
                    vehId = st.nextToken();
                    st.nextToken();//fromState
                    st.nextToken();//toState
                    st.nextToken();//commandSent
                    st.nextToken();//rcvId
                    st.nextToken();//senderId
                    rcvPltId = st.nextToken();
                    startEnd = st.nextToken();
                    
                    if(startEnd.contains("Start")) {
                        Message msg = new Message();
                        msg.time = time;
                        msg.commandSent = startEnd.split("_")[0];
                        msg.senderPltId = vehId;
                        messages.add(msg);
                    } else if (startEnd.contains("End")) {
                        for(int i = 0; i < messages.size(); i++) {
                            switch(startEnd.split("_")[0]) {
                                case "Split":
                                    if(messages.get(i).commandSent.equals(startEnd.split("_")[0])
                                        && messages.get(i).senderPltId.equals(rcvPltId)) {
                                        if(time - messages.get(i).time > threshold) ret = false;
                                        else ret = true;
                                        messages.remove(i);
                                    }
                                    break;
                                default:
                                    if(messages.get(i).commandSent.equals(startEnd.split("_")[0])
                                        && messages.get(i).senderPltId.equals(vehId)) {
                                        if(time - messages.get(i).time > threshold) ret = false;
                                        else ret = true;
                                        messages.remove(i);
                                    }
                                    break;
                            }
                            
                        }
                    }
                }
            }
        
            reader.close();
        
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        return ret;
    }
    
    private Boolean operationSuccessRateVerification(String txtdir, String nof, int threshold) {
        boolean ret = true;
        ArrayList<Message> messages = new ArrayList<>();
        int addCount = 0;
        int delCount = 0;
    
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(new File(txtdir)));
        
            String line;
            String command1;
            String command2;
            String senderPltId;
            String rcvId;
            String vehId;
            float time;
        
            while ((line = reader.readLine()) != null) {
            
                StringTokenizer st = new StringTokenizer(line, "\t ");
            
                if (st.countTokens() == 9) {
                    String temp = st.nextToken();
                    if(temp.equals("timeStep")) continue;
                
                    time = Float.valueOf(temp);
                    vehId = st.nextToken();
                    st.nextToken();
                    st.nextToken();
                    command1 = st.nextToken();
                    rcvId = st.nextToken();
                    senderPltId = st.nextToken();
                    st.nextToken();
                    command2 = st.nextToken();
                    
                    if(command1.contains("REQ")) {
                        Message msg = new Message();
                        msg.receiverId = rcvId;
                        msg.senderPltId = senderPltId;
                        msg.commandSent = command1.split("_")[0];
                        messages.add(msg);
                        addCount++;
                        continue;
                    } if(command2.contains("Start")) {
                        Message msg = new Message();
                        if(senderPltId.equals("-") || rcvId.equals("-")) continue;
                        msg.commandSent = command2.split("_")[0];
                        msg.senderPltId = senderPltId;
                        msg.receiverId = rcvId;
                        messages.add(msg);
                        addCount++;
                    }
                    
                    if (command2.contains("End")) {
                            switch(command2.split("_")[0]) {
                                case "Split":
                                    for(int i = 0; i < messages.size(); i++) {
                                        if (messages.get(i).commandSent.toLowerCase().equals("split")
                                            && messages.get(i).senderPltId.equals(rcvId) && messages.get(i).receiverId.equals(senderPltId)) {
                                            messages.remove(i);
                                            delCount++;
                                            break;
                                        }
                                    }
                                    break;
                                case "Merge":
                                    for(int i = 0; i < messages.size(); i++) {
                                        if (messages.get(i).commandSent.equals("MERGE") && messages.get(i).receiverId.equals(rcvId)) {
                                            messages.remove(i);
                                            delCount++;
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    for(int i = 0; i < messages.size(); i++) {
                                        if (messages.get(i).commandSent.equals(command2.split("_")[0])
                                            && messages.get(i).senderPltId.equals(vehId)) {
                                            messages.remove(i);
                                            delCount++;
                                            break;
                                        }
                                    }
                                    break;
                            
                            }
                        }
                    }
                }

            for(int i = 0; i <messages.size(); i++) {
//                System.out.println(messages.get(i).commandSent);
            }
//            System.out.println(delCount/addCount);
            if(delCount < addCount * threshold/100) ret = false;

            reader.close();
        
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        return ret;
    }
}
